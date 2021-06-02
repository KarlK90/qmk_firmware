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

static SerialConfig serial_config = {
    .speed = (SERIAL_USART_SPEED),  // speed - mandatory
    .cr1   = (SERIAL_USART_CR1),    // CR1
    .cr2   = (SERIAL_USART_CR2),    // CR2
#if defined(SERIAL_USART_FULL_DUPLEX)
    .cr3 = (SERIAL_USART_CR3)  // CR3
#else
    .cr3 = ((SERIAL_USART_CR3) | USART_CR3_HDSEL)  // CR3
#endif
};

void               handle_transactions_slave(void);
static inline void sdClear(SerialDriver* driver);

#if defined(SERIAL_USART_FULL_DUPLEX)

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

#else

static inline msg_t sdWriteHalfDuplex(SerialDriver* driver, const uint8_t* data, sysinterval_t size) {
    msg_t bytes_written = sdWrite(driver, data, size);

    /* Half duplex requires us to read back the data we just wrote - just throw it away */
    if (bytes_written) {
        uint8_t dump[bytes_written];
        sdRead(driver, dump, bytes_written);
    }

    return bytes_written;
}

#    undef sdWrite
#    define sdWrite sdWriteHalfDuplex

static inline msg_t sdWriteTimeoutHalfDuplex(SerialDriver* driver, const uint8_t* data, size_t size, sysinterval_t timeout) {
    msg_t bytes_written = sdWriteTimeout(driver, data, size, timeout);

    /* Half duplex requires us to read back the data we just wrote - just throw it away */
    if (bytes_written) {
        uint8_t dump[bytes_written];
        sdReadTimeout(driver, dump, bytes_written, timeout);
    }

    return bytes_written;
}

#    undef sdWriteTimeout
#    define sdWriteTimeout sdWriteTimeoutHalfDuplex

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

#endif

// clang-format off
/**
 * @brief Blocking send of buffer with timeout.
 * 
 * @return true Send success.
 * @return false Send failed.
 */
static inline bool sendTimeout(SerialDriver* driver, const uint8_t* source, size_t size, sysinterval_t timeout) {
    return (size_t)sdWriteTimeout(driver, source, size, timeout) == size;
}

/**
 * @brief Blocking send of buffer.
 * 
 * @return true Send success.
 * @return false Send failed.
 */
static inline bool send(SerialDriver* driver, const uint8_t* source, size_t size) { 
    return (size_t)sdWrite(driver, source, size) == size;
}

/**
 * @brief Blocking receive of size * bytes.
 * 
 * @return true Receive success.
 * @return false Receive failed.
 */
static inline bool receive(SerialDriver* driver, uint8_t* destination, size_t size) { 
    return (size_t)sdRead(driver, destination, size) == size; 
}

/**
 * @brief  Blocking receive of size * bytes with timeout.
 * 
 * @return true Receive success.
 * @return false Receive failed.
 */
static inline bool receiveTimeout(SerialDriver* driver, uint8_t* destination, size_t size, sysinterval_t timeout) { 
    return (size_t)sdReadTimeout(driver, destination, size, timeout) == size; 
}

// clang-format on

/**
 * @brief Clear the receive input queue.
 *
 * @param driver Driver to clear.
 */
static inline void sdClear(SerialDriver* driver) {
    /* Hard reset the input queue. */
    osalSysLock();
    iqResetI(&(driver)->iqueue);
    osalSysUnlock();
}

/*
 * This thread runs on the slave and responds to transactions initiated
 * by the master.
 */
static THD_WORKING_AREA(waSlaveThread, 2048);
static THD_FUNCTION(SlaveThread, arg) {
    (void)arg;
    chRegSetThreadName("slave_transport");

    while (true) {
        handle_transactions_slave();
    }
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
void handle_transactions_slave(void) {
    /* Wait until there is a transaction for us. */
    uint8_t sstd_index = sdGet(&SERIAL_USART_DRIVER);

    /* Sanity check that we are actually responding to a valid transaction. */
    if (sstd_index >= NUM_TOTAL_TRANSACTIONS) {
        /* Clear the receive queue, to start with a clean slate. Parts of failed transactions could still be in it. */
        sdClear(&SERIAL_USART_DRIVER);
        return;
    }

    split_transaction_desc_t* trans = &split_transaction_table[sstd_index];

    /* Send back the handshake which is XORed as a simple checksum,
     to signal that the slave is ready to receive possible transaction buffers  */
    sstd_index ^= HANDSHAKE_MAGIC;
    if (!send(&SERIAL_USART_DRIVER, &sstd_index, sizeof(sstd_index))) {
        *trans->status = TRANSACTION_DATA_ERROR;
        return;
    }

    /* Receive transaction buffer from the master. If this transaction requires it.*/
    if (trans->initiator2target_buffer_size) {
        if (!receive(&SERIAL_USART_DRIVER, split_trans_initiator2target_buffer(trans), trans->initiator2target_buffer_size)) {
            *trans->status = TRANSACTION_DATA_ERROR;
            return;
        }
    }

    /* Allow any slave processing to occur. */
    if (trans->slave_callback) {
        trans->slave_callback(trans->initiator2target_buffer_size, split_trans_initiator2target_buffer(trans), trans->initiator2target_buffer_size, split_trans_target2initiator_buffer(trans));
    }

    /* Send transaction buffer to the master. If this transaction requires it. */
    if (trans->target2initiator_buffer_size) {
        if (!send(&SERIAL_USART_DRIVER, split_trans_target2initiator_buffer(trans), trans->target2initiator_buffer_size)) {
            *trans->status = TRANSACTION_DATA_ERROR;
            return;
        }
    }

    *trans->status = TRANSACTION_ACCEPTED;

    return;
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
    uint8_t sstd_index = index;

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

    /* Clear the receive queue, to start with a clean slate. Parts of failed transactions could still be in it. */
    sdClear(&SERIAL_USART_DRIVER);

    /* Send transaction table index to the slave, which doubles as basic handshake token. */
    if (!sendTimeout(&SERIAL_USART_DRIVER, &sstd_index, (size_t)sizeof(sstd_index), TIME_MS2I(SERIAL_USART_TIMEOUT))) {
        dprintln("USART: Send Handshake failed.");
        return TRANSACTION_TYPE_ERROR;
    }

    uint8_t sstd_index_shake = 0xFF;

    /* Which we always read back first so that we can error out correctly.
     *   - due to the half duplex limitations on return codes, we always have to read *something*.
     *   - without the read, write only transactions *always* succeed, even during the boot process where the slave is not ready.
     */
    if (!receiveTimeout(&SERIAL_USART_DRIVER, &sstd_index_shake, sizeof(sstd_index_shake), TIME_MS2I(SERIAL_USART_TIMEOUT)) || (sstd_index_shake != (sstd_index ^ HANDSHAKE_MAGIC))) {
        dprintln("USART: Handshake failed.");
        return TRANSACTION_NO_RESPONSE;
    }

    /* Send transaction buffer to the slave. If this transaction requires it. */
    if (trans->initiator2target_buffer_size) {
        if (!sendTimeout(&SERIAL_USART_DRIVER, split_trans_initiator2target_buffer(trans), trans->initiator2target_buffer_size, TIME_MS2I(SERIAL_USART_TIMEOUT))) {
            dprintln("USART: Send failed.");
            return TRANSACTION_NO_RESPONSE;
        }
    }

    /* Receive transaction buffer from the slave. If this transaction requires it. */
    if (trans->target2initiator_buffer_size) {
        if (!receiveTimeout(&SERIAL_USART_DRIVER, split_trans_target2initiator_buffer(trans), trans->target2initiator_buffer_size, TIME_MS2I(SERIAL_USART_TIMEOUT))) {
            dprintln("USART: Receive failed.");
            return TRANSACTION_NO_RESPONSE;
        }
    }

    return TRANSACTION_END;
}
