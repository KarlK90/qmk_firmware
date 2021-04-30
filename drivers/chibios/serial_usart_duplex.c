#include <stdatomic.h>
#include "quantum.h"
#include "serial.h"
#include "printf.h"

#include "ch.h"
#include "hal.h"

#if !defined(USART_CR1_M0)
#    define USART_CR1_M0 USART_CR1_M  // some platforms (f1xx) dont have this so
#endif

#if !defined(USE_GPIOV1)
// The default PAL alternate modes are used to signal that the pins are used for USART
#    if !defined(SERIAL_USART_TX_PAL_MODE)
#        define SERIAL_USART_TX_PAL_MODE 7
#    endif
#    if !defined(SERIAL_USART_RX_PAL_MODE)
#        define SERIAL_USART_RX_PAL_MODE 7
#    endif
#endif

#if !defined(SERIAL_USART_DRIVER)
#    define SERIAL_USART_DRIVER UARTD1
#endif

#if !defined(SERIAL_USART_CR1)
#    define SERIAL_USART_CR1 (USART_CR1_PCE | USART_CR1_PS | USART_CR1_M0)  // parity enable, odd parity, 9 bit length
#endif

#if !defined(SERIAL_USART_CR2)
#    define SERIAL_USART_CR2 (USART_CR2_STOP_1)  // 2 stop bits
#endif

#if !defined(SERIAL_USART_CR3)
#    define SERIAL_USART_CR3 0x0
#endif

#if !defined(SERIAL_USART_TX_PIN)
#    define SERIAL_USART_TX_PIN A9
#endif

#if !defined(SERIAL_USART_RX_PIN)
#    define SERIAL_USART_RX_PIN A10
#endif

#if defined(USART1_REMAP)
#    define USART_REMAP (AFIO->MAPR |= AFIO_MAPR_USART1_REMAP);
#elif defined(USART2_REMAP)
#    define USART_REMAP (AFIO->MAPR |= AFIO_MAPR_USART2_REMAP);
#elif defined(USART3_PARTIALREMAP)
#    define USART_REMAP (AFIO->MAPR |= AFIO_MAPR_USART3_REMAP_PARTIALREMAP);
#elif defined(USART3_FULLREMAP)
#    define USART_REMAP (AFIO->MAPR |= AFIO_MAPR_USART3_REMAP_FULLREMAP);
#endif

#if !defined(SELECT_SERIAL_SPEED)
#    define SELECT_SERIAL_SPEED 1
#endif

#if defined(SERIAL_USART_SPEED)
// Allow advanced users to directly set SERIAL_USART_SPEED
#elif SERIAL_SPEED == 0
#    define SERIAL_USART_SPEED 460800
#elif SERIAL_SPEED == 1
#    define SERIAL_USART_SPEED 230400
#elif SERIAL_SPEED == 2
#    define SERIAL_USART_SPEED 115200
#elif SERIAL_SPEED == 3
#    define SERIAL_USART_SPEED 57600
#elif SERIAL_SPEED == 4
#    define SERIAL_USART_SPEED 38400
#elif SERIAL_SPEED == 5
#    define SERIAL_USART_SPEED 19200
#else
#    error invalid SERIAL_SPEED value
#endif

#if !defined(SERIAL_USART_TIMEOUT)
#    define SERIAL_USART_TIMEOUT 100
#endif

#define HANDSHAKE_MAGIC 0x7
#define HANDSHAKE_START_SEND 0x11

// clang-format off

typedef enum { 
    SIGNAL_HANDSHAKE_RECEIVED = 1,
    SIGNAL_XOR_HANDSHAKE_TRANSMITTED_TARGET,
    SIGNAL_XOR_HANDSHAKE_VALID,
    SIGNAL_START_SEND,
    SIGNAL_ERROR
} transaction_signal_t;

typedef enum { WAITING, TARGET_RECEIVED_HANDSHAKE, HANDSHAKE_SEND_INITIATOR, TRANSMITTING } transaction_state_t;

// clang-format on

static transaction_state_t  current_state = WAITING;
static atomic_uint_least8_t handshake     = ~0;
static thread_reference_t   tp_actor      = NULL;

static void handle_transaction_target(uint8_t sstd_index);
static int  handle_transaction_initiator(uint8_t sstd_index);
static void error_callback(UARTDriver* uartp, uartflags_t e);
static void transfer_complete_callback(UARTDriver* uartp);

/*
 * UART driver configuration structure. We use the blocking DMA enabled API and
 * the rxchar callback to receive handshake tokens.
 */
// clang-format off
static UARTConfig uart_config = {
    .txend1_cb = transfer_complete_callback,
    .txend2_cb = NULL,
    .rxend_cb = NULL,
    .rxchar_cb = NULL,
    .rxerr_cb = error_callback,
    .timeout_cb = NULL,
    .speed = (SERIAL_USART_SPEED),
    .cr1 = (SERIAL_USART_CR1),
    .cr2 = (SERIAL_USART_CR2),
    .cr3 = (SERIAL_USART_CR3)
};
// clang-format on

// CALLBACK FUNCTIONS
/////////////////////

static void error_callback(UARTDriver* uartp, uartflags_t e) {
    (void)uartp;
    (void)e;
    chSysLockFromISR();
    switch (current_state) {
        default:
            current_state = WAITING;
            chEvtSignalI(tp_actor, (eventmask_t)SIGNAL_ERROR);
            break;
    }
    chSysUnlockFromISR();
}

static void transfer_complete_callback(UARTDriver* uartp) {
    (void)uartp;
    chSysLockFromISR();
    switch (current_state) {
        case TARGET_RECEIVED_HANDSHAKE:
            chEvtSignalI(tp_actor, (eventmask_t)SIGNAL_XOR_HANDSHAKE_TRANSMITTED_TARGET);
            break;
        default:
            break;
    }
    chSysUnlockFromISR();
}

/*
 * This callback is invoked when a character is received but the application
 * was not ready to receive it, the character is passed as parameter.
 * Receive transaction table index from initiator, which doubles as basic handshake token. */
static void target_callback(UARTDriver* uartp, uint16_t received_handshake) {
    (void)uartp;
    chSysLockFromISR();
    switch (current_state) {
        case WAITING:
            /* Check if received handshake is not a valid transaction id.
             * Please note that we can still catch a seemingly valid handshake
             * i.e. a byte from a ongoing transfer which is in the allowed range.
             * So this check mainly prevents any obviously wrong handshakes and
             * subsequent wakeups of the receiving thread, which is a costly operation. */
            if (received_handshake > NUM_TOTAL_TRANSACTIONS) {
                return;
            }

            current_state                = TARGET_RECEIVED_HANDSHAKE;
            static uint8_t handshake_xor = 0;
            handshake                    = received_handshake;

            /* Send back the handshake which is XORed as a simple checksum,
            to signal that the target is ready to receive possible transaction buffers  */
            handshake_xor = received_handshake ^ HANDSHAKE_MAGIC;
            uartStartSendI(&SERIAL_USART_DRIVER, sizeof(handshake_xor), &handshake_xor);

            /* Wakeup receiving thread to start a transaction. */
            chEvtSignalI(tp_actor, (eventmask_t)SIGNAL_HANDSHAKE_RECEIVED);
            break;
        case TRANSMITTING:
            if (received_handshake == HANDSHAKE_START_SEND) {
                /* Wakeup receiving thread to start a transaction. */
                chEvtSignalI(tp_actor, (eventmask_t)SIGNAL_START_SEND);
            }
            break;
        default:
            break;
    }
    chSysUnlockFromISR();
}

/*
 * This callback is invoked when a character is received but the application
 * was not ready to receive it, the character is passed as parameter.
 * Receive transaction table index from initiator, which doubles as basic handshake token. */
static void initiator_callback(UARTDriver* uartp, uint16_t received_handshake) {
    chSysLockFromISR();
    switch (current_state) {
        case HANDSHAKE_SEND_INITIATOR:
            if ((received_handshake ^ HANDSHAKE_MAGIC) == handshake) {
                /* Wakeup receiving thread to start a transaction. */
                chEvtSignalI(tp_actor, (eventmask_t)SIGNAL_XOR_HANDSHAKE_VALID);
            } else {
                /* Wakeup receiving thread to start a transaction. */
                chEvtSignalI(tp_actor, (eventmask_t)SIGNAL_ERROR);
            }
            break;
        case TRANSMITTING:
            if (received_handshake == HANDSHAKE_START_SEND) {
                /* Wakeup receiving thread to start a transaction. */
                chEvtSignalI(tp_actor, (eventmask_t)SIGNAL_START_SEND);
            }
            break;
        default:
            break;
    }
    chSysUnlockFromISR();
}

// RECEIVING THREAD
///////////////////

/*
 * This thread runs on the slave half and reacts to transactions initiated from the master.
 */
static THD_WORKING_AREA(waTargetThread, 1024);
static THD_FUNCTION(TargetThread, arg) {
    (void)arg;
    chRegSetThreadName("target_usart_tx_rx");

    while (true) {
        /* We sleep as long as there is no handshake waiting for us. */
        chEvtWaitOne((eventmask_t)SIGNAL_HANDSHAKE_RECEIVED);
        handle_transaction_target(handshake);
        current_state = WAITING;
    }
}

/// INIT FUNCTIONS
//////////////////

__attribute__((weak)) void usart_init(void) {
#if defined(USE_GPIOV1)
    palSetLineMode(SERIAL_USART_TX_PIN, PAL_MODE_STM32_ALTERNATE_PUSHPULL);
    palSetLineMode(SERIAL_USART_RX_PIN, PAL_MODE_INPUT);
#else
    palSetLineMode(SERIAL_USART_TX_PIN, PAL_MODE_ALTERNATE(SERIAL_USART_TX_PAL_MODE) | PAL_STM32_OTYPE_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
    palSetLineMode(SERIAL_USART_RX_PIN, PAL_MODE_ALTERNATE(SERIAL_USART_RX_PAL_MODE) | PAL_STM32_OTYPE_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
#endif
}

void soft_serial_target_init(void) {
    usart_init();

#if defined(USART_REMAP)
    USART_REMAP
#endif

    tp_actor = chThdCreateStatic(waTargetThread, sizeof(waTargetThread), HIGHPRIO, TargetThread, NULL);

    uart_config.rxchar_cb = target_callback;
    uartStart(&SERIAL_USART_DRIVER, &uart_config);
}

void soft_serial_initiator_init(void) {
    usart_init();

#if defined(SERIAL_USART_PIN_SWAP)
    uart_config.cr2 |= USART_CR2_SWAP;  // master has swapped TX/RX pins
#endif

#if defined(USART_REMAP)
    USART_REMAP
#endif

    tp_actor              = chThdGetSelfX();
    uart_config.rxchar_cb = initiator_callback;
    uartStart(&SERIAL_USART_DRIVER, &uart_config);
}

/// TRANSACTION FUNCTIONS
/////////////////////////

/**
 * @brief React to transactions started by the master.
 * This version uses duplex send and receive usart pheriphals and DMA backed transfers.
 */
void inline handle_transaction_target(uint8_t sstd_index) {
    size_t                    buffer_size = 0;
    msg_t                     msg         = 0;
    split_transaction_desc_t* trans       = &split_transaction_table[sstd_index];

    if (chEvtWaitAnyTimeout((eventmask_t)(SIGNAL_XOR_HANDSHAKE_TRANSMITTED_TARGET | SIGNAL_ERROR), TIME_MS2I(SERIAL_USART_TIMEOUT)) != SIGNAL_XOR_HANDSHAKE_TRANSMITTED_TARGET) {
        if (trans->status) {
            *trans->status = TRANSACTION_NO_RESPONSE;
        }
        return;
    }

    current_state = TRANSMITTING;

    if (trans->initiator2target_buffer_size) {
        const uint8_t flag = HANDSHAKE_START_SEND;
        buffer_size        = (size_t)sizeof(flag);
        msg                = uartSendTimeout(&SERIAL_USART_DRIVER, &buffer_size, &flag, TIME_MS2I(SERIAL_USART_TIMEOUT));
        if (msg != MSG_OK) {
            if (trans->status) {
                *trans->status = TRANSACTION_NO_RESPONSE;
            }
            return;
        }

        /* Receive transaction buffer from the master. If this transaction requires it.*/
        buffer_size = (size_t)trans->initiator2target_buffer_size;
        msg         = uartReceiveTimeout(&SERIAL_USART_DRIVER, &buffer_size, split_trans_initiator2target_buffer(trans), TIME_MS2I(SERIAL_USART_TIMEOUT));
        if (msg != MSG_OK) {
            if (trans->status) {
                *trans->status = TRANSACTION_NO_RESPONSE;
            }
            return;
        }
    }

    // Allow any slave processing to occur
    if (trans->slave_callback) {
        trans->slave_callback(trans->initiator2target_buffer_size, split_trans_initiator2target_buffer(trans), trans->target2initiator_buffer_size, split_trans_target2initiator_buffer(trans));
    }

    if (trans->target2initiator_buffer_size) {
        if (!chEvtWaitOneTimeout((eventmask_t)SIGNAL_START_SEND, TIME_MS2I(SERIAL_USART_TIMEOUT))) {
            if (trans->status) {
                *trans->status = TRANSACTION_NO_RESPONSE;
            }
            return;
        }

        /* Send transaction buffer to the master. If this transaction requires it. */
        buffer_size = (size_t)trans->target2initiator_buffer_size;
        if (buffer_size) {
            msg = uartSendTimeout(&SERIAL_USART_DRIVER, &buffer_size, split_trans_target2initiator_buffer(trans), TIME_MS2I(SERIAL_USART_TIMEOUT));
            if (msg != MSG_OK) {
                if (trans->status) {
                    *trans->status = TRANSACTION_NO_RESPONSE;
                }
                return;
            }
        }
    }

    if (trans->status) {
        *trans->status = TRANSACTION_ACCEPTED;
    }
}

static int handle_transaction_initiator(uint8_t sstd_index) {
    handshake = sstd_index;

    if (handshake > NUM_TOTAL_TRANSACTIONS) {
        return TRANSACTION_TYPE_ERROR;
    }

    split_transaction_desc_t* const trans       = &split_transaction_table[handshake];
    msg_t                           msg         = 0;
    size_t                          buffer_size = (size_t)sizeof(handshake);
    current_state                               = HANDSHAKE_SEND_INITIATOR;

    /* Send transaction table index to the target, which doubles as basic handshake token. */
    uartStartSend(&SERIAL_USART_DRIVER, buffer_size, &handshake);

    if (chEvtWaitAnyTimeout((eventmask_t)(SIGNAL_XOR_HANDSHAKE_VALID | SIGNAL_ERROR), TIME_MS2I(SERIAL_USART_TIMEOUT)) != SIGNAL_XOR_HANDSHAKE_VALID) {
        dprintln("USART: Send Handshake Failed");
        return TRANSACTION_NO_RESPONSE;
    }

    current_state = TRANSMITTING;

    if (trans->initiator2target_buffer_size) {
        if (!chEvtWaitOneTimeout((eventmask_t)SIGNAL_START_SEND, TIME_MS2I(SERIAL_USART_TIMEOUT))) {
            dprintln("USART: Waiting for Transmission Start Failed");
            return TRANSACTION_NO_RESPONSE;
        }

        /* Send transaction buffer to the target. If this transaction requires it. */
        buffer_size = (size_t)trans->initiator2target_buffer_size;
        if (buffer_size) {
            msg = uartSendFullTimeout(&SERIAL_USART_DRIVER, &buffer_size, split_trans_initiator2target_buffer(trans), TIME_MS2I(SERIAL_USART_TIMEOUT));
            if (msg != MSG_OK) {
                dprintln("USART: Send Failed");
                return TRANSACTION_NO_RESPONSE;
            }
        }
    }

    if (trans->target2initiator_buffer_size) {
        const uint8_t flag = HANDSHAKE_START_SEND;
        buffer_size        = (size_t)sizeof(flag);
        msg                = uartSendTimeout(&SERIAL_USART_DRIVER, &buffer_size, &flag, TIME_MS2I(SERIAL_USART_TIMEOUT));
        if (msg != MSG_OK) {
            dprintln("USART: Sending Transmission Start Failed");
            return TRANSACTION_NO_RESPONSE;
        }

        /* Receive transaction buffer from the target. If this transaction requires it. */
        buffer_size = (size_t)trans->target2initiator_buffer_size;
        msg         = uartReceiveTimeout(&SERIAL_USART_DRIVER, &buffer_size, split_trans_target2initiator_buffer(trans), TIME_MS2I(SERIAL_USART_TIMEOUT));
        if (msg != MSG_OK) {
            dprintln("USART: Receive Failed");
            return TRANSACTION_NO_RESPONSE;
        }
    }

    return TRANSACTION_END;
}

/**
 * @brief Start transaction from initiator to target.
 * This version uses duplex send and receive usart pheriphals and DMA backed transfers.
 *
 * @param index Transaction Table index of the transaction to start.
 * @return int TRANSACTION_NO_RESPONSE in case of Timeout.
 *             TRANSACTION_TYPE_ERROR in case of invalid transaction index.
 *             TRANSACTION_END in case of success.
 */
int soft_serial_transaction(int index) {
    int result    = handle_transaction_initiator(index);
    current_state = WAITING;
    return result;
}
