#pragma once

#include "config_common.h"

/* USB CONFIG */
#define USB_POLLING_INTERVAL_MS 1
#define FORCE_NKRO

/* Device descriptor parameters */
#define VENDOR_ID  0x445A   // DZ
#define PRODUCT_ID 0x5A59  // ZY
#define DEVICE_VER 0x0001
#define MANUFACTURER KARLK90

// clang-format off
#if defined(__riscv)
#    define PRODUCT DZZY RISC-V
#elif defined(__arm__)
#    define PRODUCT DZZY ARM
#endif
#define DESCRIPTION Barebones Gamepad
// clang-format on

/* MATRIX CONFIG */
#define MATRIX_ROWS 5
#define MATRIX_COLS 6
#define MATRIX_COL_PINS \
    { A0, A1, A2, A3, A4, A5 }
#define MATRIX_ROW_PINS \
    { B0, B1, B2, B4, B5 }

#define DIODE_DIRECTION ROW2COL
#define DEBUG_MATRIX_SCAN_RATE
#define DEBOUNCE 5
#define BUSY_WAIT
#define BUSY_WAIT_INSTRUCTIONS 10

#if defined(__riscv)
#    define GPIO_INPUT_PIN_DELAY 10
#elif defined(__arm__)
#    define GPIO_INPUT_PIN_DELAY 10
#endif

#define QMK_KEYS_PER_SCAN 8

/* SPI DRIVER */
#define EXTERNAL_EEPROM_SPI_SLAVE_SELECT_PIN B12

/* TAPPING CONFIG */
/*#define NO_ACTION_TAPPING
#define NO_ACTION_ONESHOT*/