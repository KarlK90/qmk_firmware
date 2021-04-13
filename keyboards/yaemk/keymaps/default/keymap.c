#include QMK_KEYBOARD_H

enum Layers {
    _BASE = 0,
    _FN   = 1,
    _RGB  = 2,
    _GAME = 3,
};

enum Encoder {
    _ENCODER_LEFT  = 0,
    _ENCODER_RIGHT = 1,
};

// clang-format off

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [_BASE] = LAYOUT( \
     KC_ESC         , KC_1          , KC_2          , KC_3          , KC_4          , KC_5          , LCTL(KC_GRV)  ,                                 KC_ESC        , KC_6          , KC_7          , KC_8          , KC_9          , KC_0          , KC_MINS       ,\
     KC_TAB         , KC_Q          , KC_W          , KC_E          , KC_R          , KC_T          , MO(_RGB)      ,                                 MO(_RGB)      , KC_Y          , KC_U          , KC_I          , KC_O          , KC_P          , KC_LBRC       ,\
     KC_CAPS        , LGUI_T(KC_A)  , LALT_T(KC_S)  , LCTL_T(KC_D)  , LSFT_T(KC_F)  , KC_G          , MO(_FN)       ,                                 MO(_FN)       , KC_H          , LSFT_T(KC_J)  , LCTL_T(KC_K)  , LALT_T(KC_L)  ,LGUI_T(KC_SCLN), KC_QUOT       ,\
     KC_NUBS        , KC_Z          , KC_X          , KC_C          , KC_V          , KC_B          , KC_BSPC       , KC_BSPC       , KC_ENT        , KC_ENT        , KC_N          , KC_M          , KC_COMM       , KC_DOT        , KC_SLSH       , KC_NUBS       ,\
                                                      DF(_GAME)     , KC_ESC        , KC_CAPS       , KC_BSPC       , KC_LGUI       , KC_CAPS       , KC_ENT        , MT(KC_CAPS,KC_SPC), KC_LALT   , DF(_GAME)\
  ),
   [_FN] = LAYOUT( \
    _______         , _______       , KC_ACL0       , KC_ACL1       , KC_ACL2       , RESET         , _______       ,                                 _______       , RESET         , KC_F10        , KC_F11        , KC_F12        , _______       , _______       ,\
    _______         , _______       , KC_BTN1       , KC_MS_U       , KC_BTN2       , KC_PSCR       , _______       ,                                 _______       , _______       , KC_F7         , KC_F8         , KC_F9         , _______       , _______       ,\
    _______         , _______       , KC_MS_L       , KC_MS_D       , KC_MS_R       , _______       , _______       ,                                 _______       , _______       , KC_F4         , KC_F5         , KC_F6         , _______       , _______       ,\
    _______         , _______       , _______       , _______       , _______       , _______       , _______       , _______       , _______       , _______       , _______       , KC_F1         , KC_F2         , KC_F3         , _______       , _______       ,\
                                                      _______       , _______       , _______       , _______       , _______       , _______       , _______       , _______       , _______       , _______ \
  ),
  [_RGB] = LAYOUT( \
    _______         , _______       , _______       , _______       , _______       , _______       , _______       ,                                 _______       , _______       , RGB_TOG       , RGB_M_SW      , _______       , _______       , _______       ,\
    _______         , _______       , _______       , _______       , _______       , _______       , _______       ,                                 _______       , _______       , RGB_HUI       , RGB_HUD       , RGB_SAI       , RGB_SAD       , _______       ,\
    _______         , _______       , _______       , _______       , _______       , _______       , _______       ,                                 _______       , _______       , RGB_MOD       , RGB_RMOD      , RGB_SPI       , RGB_SPD       , _______       ,\
    _______         , _______       , _______       , _______       , _______       , _______       , _______       , _______       , _______       , _______       , _______       , RGB_VAI       , RGB_VAD       , _______       , _______       , _______       ,\
                                                      _______       , _______       , _______       , _______       , _______       , _______       , _______       , _______       , _______       , _______ \
  ),
  [_GAME] = LAYOUT( \
     KC_ESC         , KC_1          , KC_2          , KC_3          , KC_4          , KC_5          , LCTL(KC_GRV)  ,                                 KC_ESC        , KC_6          , KC_7          , KC_8          , KC_9          , KC_0          , KC_MINS       ,\
     KC_TAB         , KC_LSFT       , KC_Q          , KC_X          , KC_E          , KC_R          , KC_T          ,                                 _______       , KC_Y          , KC_U          , KC_I          , KC_O          , KC_P          , KC_LBRC       ,\
     KC_LSFT        , KC_LSFT       , KC_A          , KC_W          , KC_D          , KC_F          , KC_G          ,                                 _______       , KC_H          , KC_J          , KC_K          , KC_L          , KC_SCLN       , KC_QUOT       ,\
     KC_LALT        , KC_LCTL       , KC_Z          , KC_S          , KC_C          , KC_V          , KC_SPC        , KC_BSPC       , KC_BSPC       , KC_ENT        , KC_ENT        , KC_N          , KC_M          , KC_COMM       , KC_DOT        , KC_SLSH       ,\
                                                      _______       , KC_LCTL       , KC_SPC        , KC_SPC        , KC_LGUI       , KC_CAPS       , KC_ENT        , KC_SPC        , KC_LALT       , _______
  )
};

// clang-format on

#if defined(ENCODER_ENABLE)
void encoder_update_user(uint8_t index, bool clockwise) {
    layer_state_t current_state = get_highest_layer(layer_state);

    if (index == _ENCODER_LEFT) {
        if (clockwise) {
            switch (current_state) {
                case _BASE:
                default:
                    tap_code(KC_MS_WH_DOWN);
                    break;
                case _FN:
                    tap_code_delay(KC_AUDIO_VOL_UP, 10);
                    break;
                case _RGB:
                    tap_code(KC_PGUP);
                    break;
            }
        } else {
            switch (current_state) {
                case _BASE:
                default:
                    tap_code(KC_MS_WH_UP);
                    break;
                case _FN:
                    tap_code_delay(KC_AUDIO_VOL_DOWN, 10);
                    break;
                case _RGB:
                    tap_code(KC_PGDN);
                    break;
            }
        }
    } else if (index == _ENCODER_RIGHT) {
        if (clockwise) {
            switch (current_state) {
                case _BASE:
                default:
                    tap_code(KC_MS_WH_DOWN);
                    break;
                case _FN:
                    tap_code_delay(KC_MEDIA_NEXT_TRACK, 10);
                    break;
                case _RGB:
                    tap_code(KC_PGUP);
                    break;
            }
        } else {
            switch (current_state) {
                case _BASE:
                default:
                    tap_code(KC_MS_WH_UP);
                    break;
                case _FN:
                    tap_code_delay(KC_MEDIA_PREV_TRACK, 10);
                    break;
                case _RGB:
                    tap_code(KC_PGDN);
                    break;
            }
        }
    }
}
#endif

#if defined(OLED_DRIVER_ENABLE)

#    define w_val(str, val)                         \
        snprintf(buffer, sizeof(buffer), str, val); \
        oled_write_ln(buffer, false)

void render_display(void) {
    static layer_state_t last_state    = ~0;
    layer_state_t        current_state = get_highest_layer(layer_state);

    if (last_state != current_state) {
        oled_clear();
        last_state = current_state;
    }

    char buffer[25];
    switch (current_state) {
        case _BASE:
            oled_write_ln("QWERTY", false);
            break;
        case _FN:
            oled_write_ln("Function", false);
#    if defined(DEBUG_MATRIX_SCAN_RATE)
            oled_write_ln("", false);
            oled_write_ln("Scan Freq:", false);
            w_val("%05ld", get_matrix_scan_rate());
#    endif
            break;
        case _RGB:
#    if defined(RGB_MATRIX_ENABLE)
            oled_write_ln("RGB", false);
            oled_write_ln("", false);
            w_val("En:   %03d\n", rgb_matrix_config.enable);
            w_val("Mode: %03d\n", rgb_matrix_config.mode);
            w_val("Hue:  %03d\n", rgb_matrix_config.hsv.h);
            w_val("Sat:  %03d\n", rgb_matrix_config.hsv.s);
            w_val("Bri:  %03d\n", rgb_matrix_config.hsv.v);
            w_val("Spd:  %03d\n", rgb_matrix_config.speed);
            break;
#    endif
        case _GAME:
            oled_write_ln("Game", false);
            break;
        default:
            break;
    }
}
#endif
