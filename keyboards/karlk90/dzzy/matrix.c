#include "quantum.h"
#include <hal.h>
#include <string.h>

static const pin_t row_pins[MATRIX_ROWS] = MATRIX_ROW_PINS;
static const pin_t col_pins[MATRIX_COLS] = MATRIX_COL_PINS;

void matrix_init_custom(void) {
    for (size_t i = 0; i < MATRIX_COLS; i++) {
        setPinInputLow(col_pins[i]);
    }
    for (size_t i = 0; i < MATRIX_ROWS; i++) {
        setPinOutput(row_pins[i]);
        writePinLow(row_pins[i]);
    }
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    matrix_row_t raw_matrix[MATRIX_ROWS];

    for (size_t row_idx = 0; row_idx < MATRIX_ROWS; row_idx++) {
        writePinHigh(row_pins[row_idx]);
        while (readPin(row_pins[row_idx]) != 1)
            ;

        raw_matrix[row_idx] = (matrix_row_t)palReadGroup(GPIOA, 0x3F, 0);

        writePinLow(row_pins[row_idx]);

        /* Wait until col pins are low again. */
        size_t counter = 0xFF;
        while ((palReadGroup(GPIOA, 0x3F, 0) != 0) && counter != 0) {
            counter--;
        }
    }

    if (memcmp(current_matrix, raw_matrix, sizeof(raw_matrix)) != 0) {
        memcpy(current_matrix, raw_matrix, sizeof(raw_matrix));
        return true;
    }

    return false;
}