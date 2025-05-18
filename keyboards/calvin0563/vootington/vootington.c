// Copyright 2025 Stefan Kerkmann (@karlk90)
// SPDX-License-Identifier: GPL-2.0-or-later

#include <quantum.h>
#include <util.h>
#include <rgb_matrix.h>

#define ADC_GRP_NUM_CHANNELS (7)
#define ADC_INTERNAL_RESOLUTION (4096 - 1)
#define AMUX_NUM_CHANNELS (8)
#define SAMPLE_COUNT (10)

typedef struct {
    adcsample_t min;
    adcsample_t max;
} adc_key_config_t;

typedef struct {
    adc_key_config_t key_config[AMUX_NUM_CHANNELS][ADC_GRP_NUM_CHANNELS];
} PACKED eeprom_config_t;

_Static_assert(sizeof(eeprom_config_t) == EECONFIG_KB_DATA_SIZE, "Mismatch in keyboard EECONFIG stored data");

static eeprom_config_t config                                              = {0};
static adcsample_t     matrix_raw[AMUX_NUM_CHANNELS][ADC_GRP_NUM_CHANNELS] = {0};
static void            sample_raw_matrix(void);

pin_t adc_pins[ADC_GRP_NUM_CHANNELS] = {
    A3, // ADC0 - ADC1_IN3
    A7, // ADC1 - ADC1_IN7
    A6, // ADC2 - ADC1_IN6
    A5, // ADC3 - ADC1_IN5
    A4, // ADC4 - ADC1_IN4
    A2, // ADC5 - ADC1_IN2
    B0, // ADC6 - ADC1_IN8
};

pin_t select_pins[3] = {
    B10, // SELECT0
    B2,  // SELECT1
    B1,  // SELECT2
};

#define NO_KEY {255, 255}
// clang-format off
static const keypos_t adc_matrix_lut[AMUX_NUM_CHANNELS][ADC_GRP_NUM_CHANNELS] = {
//    ADC1_IN2 (5)    ADC1_IN3 (0)    ADC1_IN4 (4)    ADC1_IN5 (3)    ADC1_IN6 (2)    ADC1_IN7 (1)    ADC1_IN8 (6)
    { {0, 4},         {10, 3},        {2, 4},         {4, 4},         {6, 1},         {8,  4},        {2, 0}, }, // A0
    { {0, 1},         {10, 4},        {2, 1},         {4, 1},         {6, 2},         {9,  1},        {0, 0}, }, // A1
    { {0, 2},         {10, 2},        {2, 2},         {4, 2},         NO_KEY,         {8,  2},        {1, 0}, }, // A2
    { {0, 3},         {11, 3},        {2, 3},         {4, 3},         {6, 3},         {8,  3},        NO_KEY, }, // A3,
    { {1, 2},         NO_KEY,         {3, 2},         {5, 2},         {7, 2},         {9,  2},        {3, 0}, }, // A4,
    { {1, 3},         {11, 4},        {3, 3},         {5, 3},         {7, 3},         {9,  3},        NO_KEY, }, // A5,
    { {1, 1},         {11, 2},        {3, 1},         {5, 1},         {7, 1},         {10, 1},        {4, 0}, }, // A6,
    { {1, 4},         {11, 1},        {3, 4},         {7, 4},         {8, 1},         {9,  4},        {5, 0}, }, // A7,
};
// clang-format on

static void select_mux_input(uint8_t addr) {
    gpio_write_pin(select_pins[0], addr & BIT32(0));
    gpio_write_pin(select_pins[1], addr & BIT32(1));
    gpio_write_pin(select_pins[2], addr & BIT32(2));
}

void matrix_init_custom(void) {
    for (int select_pin = 0; select_pin < ARRAY_SIZE(select_pins); select_pin++) {
        gpio_set_pin_output_push_pull(select_pins[select_pin]);
        gpio_write_pin_low(select_pins[select_pin]);
    }

    for (int adc_pin = 0; adc_pin < ARRAY_SIZE(adc_pins); adc_pin++) {
        palSetLineMode(adc_pins[adc_pin], PAL_MODE_INPUT_ANALOG);
    }
    adcStart(&ADCD1, NULL);

    for (int s = 0; s < SAMPLE_COUNT; s++) {
        sample_raw_matrix();
        for (int i = 0; i < AMUX_NUM_CHANNELS; i++) {
            for (int j = 0; j < ADC_GRP_NUM_CHANNELS; j++) {
                config.key_config[i][j].min += matrix_raw[i][j];

                if (s == (SAMPLE_COUNT - 1)) {
                    config.key_config[i][j].min /= SAMPLE_COUNT;
                    config.key_config[i][j].max = config.key_config[i][j].min + config.key_config[i][j].min / 8;
                }
            }
        }
    }
}

static const ADCConversionGroup adc_config = {
    .circular     = FALSE,
    .num_channels = ADC_GRP_NUM_CHANNELS,
    .cfgr1        = ADC_CFGR1_CONT | ADC_CFGR1_RES_12BIT,
    .smpr         = ADC_SMPR_SMP1_160P5,
    .chselr       = ADC_CHSELR_CHSEL2 | ADC_CHSELR_CHSEL3 | ADC_CHSELR_CHSEL4 | ADC_CHSELR_CHSEL5 | ADC_CHSELR_CHSEL6 | ADC_CHSELR_CHSEL7 | ADC_CHSELR_CHSEL8,
};

static void sample_raw_matrix(void) {
    for (uint8_t addr = 0; addr < AMUX_NUM_CHANNELS; addr++) {
        select_mux_input(addr);
        if (adcConvert(&ADCD1, &adc_config, matrix_raw[addr], 1) != MSG_OK) {
            printf("ADC conversion failed\n");
            for (int i = 0; i < ADC_GRP_NUM_CHANNELS; i++) {
                matrix_raw[addr][i] = config.key_config[addr][i].min;
            }
        }
    }
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    static uint32_t ts;
    const bool      debug_adc = false;
    const bool      print     = timer_elapsed32(ts) > 1000;
    if (print) {
        ts = timer_read32();
    }

    sample_raw_matrix();
    bool changed = false;

    for (int i = 0; i < AMUX_NUM_CHANNELS; i++) {
        for (int j = 0; j < ADC_GRP_NUM_CHANNELS; j++) {
            const keypos_t         matrix_pos = adc_matrix_lut[i][j];
            const adc_key_config_t conf       = config.key_config[i][j];

            if (matrix_pos.row == 255 || matrix_pos.col == 255) {
                continue;
            }

            adcsample_t* sample = &matrix_raw[i][j];
            *sample             = MAX(conf.min, *sample);
            *sample             = MIN(conf.max, *sample);
            *sample             = (*sample - conf.min) * ADC_INTERNAL_RESOLUTION / (conf.max - conf.min);

            matrix_row_t old = current_matrix[matrix_pos.row];
            if (*sample > (ADC_INTERNAL_RESOLUTION / 4)) {
                if (print && debug_adc) {
                    dprintf("ADC %d %d: pressed: %d\n", i, j, *sample);
                }
                current_matrix[matrix_pos.row] |= BIT32(matrix_pos.col);
            } else {
                if (print && debug_adc) {
                    dprintf("ADC %d %d: %d\n", i, j, *sample);
                }
                current_matrix[matrix_pos.row] &= ~BIT32(matrix_pos.col);
            }
            changed |= old != current_matrix[matrix_pos.row];
        }
    }

    return changed;
}

void keyboard_post_init_kb(void) {
    // debug_enable = true;
    // debug_matrix = true;
    // debug_keyboard = true;
    // debug_mouse    = true;
    keyboard_post_init_user();
}
