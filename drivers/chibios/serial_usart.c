/* Copyright 2021 QMK
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "serial_usart.h"

#if defined(SERIAL_USART_CONFIG)
static SerialConfig serial_config = SERIAL_USART_CONFIG;
#else
static SerialConfig serial_config = {
    .speed = (SERIAL_USART_SPEED), /* speed - mandatory */
    .cr1   = (SERIAL_USART_CR1),
    .cr2   = (SERIAL_USART_CR2),
#    if !defined(SERIAL_USART_FULL_DUPLEX)
    .cr3   = ((SERIAL_USART_CR3) | USART_CR3_HDSEL) /* activate half-duplex mode */
#    else
    .cr3 = (SERIAL_USART_CR3)
#    endif
};
#endif

static inline bool react_to_transactions(void);
static inline bool receive(uint8_t* destination, const size_t size);
static inline bool send(const uint8_t* source, const size_t size);
static inline int  initiate_transaction(uint8_t sstd_index);
static inline void usart_discard_bytes(size_t n);
static inline void usart_reset(void);

/**
 * @brief   Discard bytes from the serial driver input queue, it is used in half-duplex mode.
 * @details This function is a copy of iq_read, without the memcpy calls to save some cycles.
 *
 * @param n Bytes to discard from the input queue.
 */
static void usart_discard_bytes(size_t n) {
    input_queue_t* iqp = &SERIAL_USART_DRIVER.iqueue;
    size_t         s1, s2;
    size_t         bytes_to_discard = n;

    osalDbgCheck(n > 0U);
    while (bytes_to_discard > 0) {
        osalSysLock();
        /* Number of bytes that can be read in a single atomic operation.*/
        if (n > iqGetFullI(iqp)) {
            n = iqGetFullI(iqp);
        }

        /* Number of bytes before buffer limit.*/
        s1 = (size_t)(iqp->q_top - iqp->q_rdptr);

        if (n < s1) {
            iqp->q_rdptr += n;
        } else if (n > s1) {
            s2           = n - s1;
            iqp->q_rdptr = iqp->q_buffer + s2;
        } else {
            iqp->q_rdptr = iqp->q_buffer;
        }

        iqp->q_counter -= n;
        osalSysUnlock();

        bytes_to_discard -= n;
        n = bytes_to_discard;
    }
}

/**
 * @brief Blocking send of buffer with timeout.
 *
 * @return true Send success.
 * @return false Send failed.
 */
static inline bool send(const uint8_t* source, const size_t size) {
    bool success = (size_t)sdWriteTimeout(&SERIAL_USART_DRIVER, source, size, TIME_MS2I(SERIAL_USART_TIMEOUT)) == size;

#if !defined(SERIAL_USART_FULL_DUPLEX)
    if (success) {
        /* Half duplex requires us to read back the data we just wrote - just throw it away */
        usart_discard_bytes(size);
    }

#endif

    return success;
}

/**
 * @brief  Blocking receive of size * bytes with timeout.
 *
 * @return true Receive success.
 * @return false Receive failed.
 */
static inline bool receive(uint8_t* destination, const size_t size) {
    size_t success = (size_t)sdReadTimeout(&SERIAL_USART_DRIVER, destination, size, TIME_MS2I(SERIAL_USART_TIMEOUT)) == size;
    return success;
}

/**
 * @brief Reset the receive input queue.
 */
static inline void usart_reset(void) {
    /* Hard reset the input queue. */
    osalSysLock();
    iqResetI(&(SERIAL_USART_DRIVER.iqueue));
    osalSysUnlock();
}

#if !defined(SERIAL_USART_FULL_DUPLEX)

/**
 * @brief Initiate pins for USART peripheral. Half-duplex configuration.
 */
__attribute__((weak)) void usart_init(void) {
#    if defined(USE_GPIOV1)
    palSetLineMode(SERIAL_USART_TX_PIN, PAL_MODE_STM32_ALTERNATE_OPENDRAIN);
#    else
    palSetLineMode(SERIAL_USART_TX_PIN, PAL_MODE_ALTERNATE(SERIAL_USART_TX_PAL_MODE) | PAL_STM32_OTYPE_OPENDRAIN);
#    endif

#    if defined(USART_REMAP)
    USART_REMAP;
#    endif
}

#else

/**
 * @brief Initiate pins for USART peripheral. Full-duplex configuration.
 */
__attribute__((weak)) void usart_init(void) {
#    if defined(USE_GPIOV1)
    palSetLineMode(SERIAL_USART_TX_PIN, PAL_MODE_STM32_ALTERNATE_PUSHPULL);
    palSetLineMode(SERIAL_USART_RX_PIN, PAL_MODE_INPUT);
#    else
    palSetLineMode(SERIAL_USART_TX_PIN, PAL_MODE_ALTERNATE(SERIAL_USART_TX_PAL_MODE) | PAL_STM32_OTYPE_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
    palSetLineMode(SERIAL_USART_RX_PIN, PAL_MODE_ALTERNATE(SERIAL_USART_TX_PAL_MODE) | PAL_STM32_OTYPE_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
#    endif

#    if defined(USART_REMAP)
    USART_REMAP;
#    endif
}

#endif

/*
 * This thread runs on the slave and responds to transactions initiated
 * by the master.
 */
static THD_WORKING_AREA(waSlaveThread, 1024);
static THD_FUNCTION(SlaveThread, arg) {
    (void)arg;
    chRegSetThreadName("usart_tx_rx");

    while (true) {
        if (!react_to_transactions()) {
            /* Clear the receive queue, to start with a clean slate.
             * Parts of failed transactions could still be in it. */
            usart_reset();
        }
    }
}

/**
 * @brief Slave specific initializations.
 */
void soft_serial_target_init(void) {
    usart_init();

    sdStart(&SERIAL_USART_DRIVER, &serial_config);

    /* Start transport thread. */
    chThdCreateStatic(waSlaveThread, sizeof(waSlaveThread), HIGHPRIO, SlaveThread, NULL);
}

/**
 * @brief React to transactions started by the master.
 */
static inline bool react_to_transactions(void) {
    /* Wait until there is a transaction for us. */
    uint8_t sstd_index = (uint8_t)sdGet(&SERIAL_USART_DRIVER);

    /* Sanity check that we are actually responding to a valid transaction. */
    if (sstd_index >= NUM_TOTAL_TRANSACTIONS) {
        return false;
    }

    split_transaction_desc_t* trans = &split_transaction_table[sstd_index];

    /* Send back the handshake which is XORed as a simple checksum,
     to signal that the slave is ready to receive possible transaction buffers  */
    sstd_index ^= HANDSHAKE_MAGIC;
    if (!send(&sstd_index, sizeof(sstd_index))) {
        *trans->status = TRANSACTION_DATA_ERROR;
        return false;
    }

    /* Receive transaction buffer from the master. If this transaction requires it.*/
    if (trans->initiator2target_buffer_size) {
        if (!receive(split_trans_initiator2target_buffer(trans), trans->initiator2target_buffer_size)) {
            *trans->status = TRANSACTION_DATA_ERROR;
            return false;
        }
    }

    /* Allow any slave processing to occur. */
    if (trans->slave_callback) {
        trans->slave_callback(trans->initiator2target_buffer_size, split_trans_initiator2target_buffer(trans), trans->initiator2target_buffer_size, split_trans_target2initiator_buffer(trans));
    }

    /* Send transaction buffer to the master. If this transaction requires it. */
    if (trans->target2initiator_buffer_size) {
        if (!send(split_trans_target2initiator_buffer(trans), trans->target2initiator_buffer_size)) {
            *trans->status = TRANSACTION_DATA_ERROR;
            return false;
        }
    }

    *trans->status = TRANSACTION_ACCEPTED;

    return true;
}

/**
 * @brief Master specific initializations.
 */
void soft_serial_initiator_init(void) {
    usart_init();

#if defined(SERIAL_USART_PIN_SWAP)
    serial_config.cr2 |= USART_CR2_SWAP;  // master has swapped TX/RX pins
#endif

    sdStart(&SERIAL_USART_DRIVER, &serial_config);
}

/**
 * @brief Start transaction from the master half to the slave half.
 *
 * @param index Transaction Table index of the transaction to start.
 * @return int TRANSACTION_NO_RESPONSE in case of Timeout.
 *             TRANSACTION_TYPE_ERROR in case of invalid transaction index.
 *             TRANSACTION_END in case of success.
 */
int soft_serial_transaction(int index) {
    int result = initiate_transaction((uint8_t)index);

    if (result != TRANSACTION_END) {
        /* Clear the receive queue, to start with a clean slate.
         * Parts of failed transactions could still be in it. */
        usart_reset();
    }

    return result;
}

/**
 * @brief Initiate transaction to slave half.
 */
static inline int initiate_transaction(uint8_t sstd_index) {
    /* Sanity check that we are actually starting a valid transaction. */
    if (sstd_index >= NUM_TOTAL_TRANSACTIONS) {
        dprintln("USART: Illegal transaction Id.");
        return TRANSACTION_TYPE_ERROR;
    }

    split_transaction_desc_t* trans = &split_transaction_table[sstd_index];

    /* Transaction is not registered. Abort. */
    if (!trans->status) {
        dprintln("USART: Transaction not registered.");
        return TRANSACTION_TYPE_ERROR;
    }

    /* Send transaction table index to the slave, which doubles as basic handshake token. */
    if (!send(&sstd_index, sizeof(sstd_index))) {
        dprintln("USART: Send Handshake failed.");
        return TRANSACTION_TYPE_ERROR;
    }

    uint8_t sstd_index_shake = 0xFF;

    /* Which we always read back first so that we can error out correctly.
     *   - due to the half duplex limitations on return codes, we always have to read *something*.
     *   - without the read, write only transactions *always* succeed, even during the boot process where the slave is not ready.
     */
    if (!receive(&sstd_index_shake, sizeof(sstd_index_shake)) || (sstd_index_shake != (sstd_index ^ HANDSHAKE_MAGIC))) {
        dprintln("USART: Handshake failed.");
        return TRANSACTION_NO_RESPONSE;
    }

    /* Send transaction buffer to the slave. If this transaction requires it. */
    if (trans->initiator2target_buffer_size) {
        if (!send(split_trans_initiator2target_buffer(trans), trans->initiator2target_buffer_size)) {
            dprintln("USART: Send failed.");
            return TRANSACTION_NO_RESPONSE;
        }
    }

    /* Receive transaction buffer from the slave. If this transaction requires it. */
    if (trans->target2initiator_buffer_size) {
        if (!receive(split_trans_target2initiator_buffer(trans), trans->target2initiator_buffer_size)) {
            dprintln("USART: Receive failed.");
            return TRANSACTION_NO_RESPONSE;
        }
    }

    return TRANSACTION_END;
}
