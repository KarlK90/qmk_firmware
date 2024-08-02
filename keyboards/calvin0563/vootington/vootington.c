#include <quantum.h>

#define ADC_GRP_NUM_CHANNELS (7 - 3) // FIXME!
#define ADC_GRP_BUF_DEPTH 1

static adcsample_t adc_sample_buffer[CACHE_SIZE_ALIGN(adcsample_t, ADC_GRP_NUM_CHANNELS *ADC_GRP_BUF_DEPTH)];

static const ADCConversionGroup adcgrpcfg = {
    .circular     = FALSE,
    .num_channels = ADC_GRP_NUM_CHANNELS,
    .end_cb       = NULL,
    .error_cb     = NULL,
    .cfgr1        = ADC_CFGR1_CONT | ADC_CFGR1_RES_12BIT,                                         /* CFGR1 */
    .tr           = ADC_TR(0, 0),                                                                 /* TR */
    .smpr         = ADC_SMPR_SMP_7P5,                                                             /* SMPR */
    .chselr       = ADC_CHSELR_CHSEL3 | ADC_CHSELR_CHSEL7 | ADC_CHSELR_CHSEL8 | ADC_CHSELR_CHSEL9 /* FIXME! CHSELR */
};

pin_t adc_pins[ADC_GRP_NUM_CHANNELS + 3 /* FIXME! */] = {
    A7,  // ADC0 - ADC_IN7
    B10, // ADC1 - FIXME! NOT ADC PIN
    B2,  // ADC2 - FIXME! NOT ADC PIN
    B1,  // ADC3 - ADC_IN9
    B0,  // ADC4 - ADC_IN8
    A3,  // ADC5 - ADC_IN3
    B11, // ADC6 - FIXME! NOT ADC PIN
};

pin_t select_pins[3] = {
    A6, // SELECT0
    A5, // SELECT1
    A4, // SELECT2
};

static void select_mux_input(uint8_t addr) {
    gpio_write_pin(select_pins[0], (addr >> 0) & 1);
    gpio_write_pin(select_pins[1], (addr >> 1) & 1);
    gpio_write_pin(select_pins[2], (addr >> 2) & 1);
}

extern matrix_row_t raw_matrix[MATRIX_ROWS]; // raw values
extern matrix_row_t matrix[MATRIX_ROWS];     // debounced values

// Custom matrix init function
void matrix_init_custom(void) {
    for (int select_pin = 0; select_pin < ARRAY_SIZE(select_pins); select_pin++) {
        gpio_set_pin_output_push_pull(select_pins[select_pin]);
        gpio_write_pin_low(select_pins[select_pin]);
    }
    dprintf("Select pins initialized\n");

    for (int adc_pin = 0; adc_pin < ARRAY_SIZE(adc_pins); adc_pin++) {
        if (adc_pin == 1 || adc_pin == 2 || adc_pin == 6) {
            // FIXME! Skip non-ADC pins
            continue;
        }
        palSetLineMode(adc_pins[adc_pin], PAL_MODE_INPUT_ANALOG);
    }
    dprintf("ADC initialized\n");

    adcStart(&ADCD1, NULL);
}

// Custom matrix scan function
bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    static uint32_t ts;

    const bool print = timer_elapsed32(ts) > 1000;

    if (print) dprintf("ADC values:\n");
#if 0
    for (int addr = 0; addr < 8; addr++) {
        select_mux_input(addr);
        wait_us(1);
        if (print) {
            dprintf("Addr: %d\n", addr);
            print_bin32(palReadPort(GPIOA));
            println("");
        }
        adcConvert(&ADCD1, &adcgrpcfg, adc_sample_buffer, ADC_GRP_BUF_DEPTH);
        for (int i = 0; i < ARRAY_SIZE(adc_sample_buffer); i++) {
            if (print) dprintf("adc_sample_buffer[%d]: %d\n", i, adc_sample_buffer[i]);
        }
    }
#else
    int addr = 7;
    if (print) dprintf("Addr: %d\n", addr);
    select_mux_input(addr);
    wait_us(1);
    adcConvert(&ADCD1, &adcgrpcfg, adc_sample_buffer, ADC_GRP_BUF_DEPTH);

    if (print) {
        for (int i = 0; i < ARRAY_SIZE(adc_sample_buffer); i++) {
            dprintf("adc_sample_buffer[%d]: %d\n", i, adc_sample_buffer[i]);
        }
    }
#endif
    if (print) ts = timer_read32();

    return false;
}

// Bootmagic overriden to avoid conflicts with EC
void bootmagic_scan(void) {
    ;
}

void keyboard_post_init_kb(void) {
    debug_enable   = true;
    debug_matrix   = true;
    debug_keyboard = true;
    debug_mouse    = true;
    keyboard_post_init_user();
}
