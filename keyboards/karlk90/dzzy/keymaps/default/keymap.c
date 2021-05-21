#include QMK_KEYBOARD_H
#include "keymap_us_international.h"

enum Layers {
    _BASE = 0,
    _L1 = 1,
    _L2 = 2,
    _L3 = 3
};

// clang-format off

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [_BASE] = LAYOUT( \
     KC_ESC         , KC_1          , KC_2          , KC_3          , KC_4          , KC_5          ,\
     KC_TAB         , KC_Q          , KC_W          , KC_E          , KC_R          , KC_T          ,\
     KC_LSFT        , KC_A          , KC_S          , KC_D          , KC_F          , KC_G          ,\
     KC_LSFT        , KC_Z          , KC_X          , KC_C          , KC_V          , KC_B          ,\
     KC_LCTL        , KC_NO                                         , KC_SPC       , KC_SPC        
  ),
  [_L1] = LAYOUT( \
     KC_ESC         , KC_1          , KC_2          , KC_3          , KC_4          , KC_5          ,\
     KC_TAB         , KC_Q          , KC_W          , KC_E          , KC_R          , KC_T          ,\
     KC_LSFT        , KC_A          , KC_S          , KC_D          , KC_F          , KC_G          ,\
     KC_LSFT        , KC_Z          , KC_X          , KC_C          , KC_V          , KC_B          ,\
     KC_LCTL        , KC_NO                                         , KC_SPC       , KC_SPC        
  ),
  [_L2] = LAYOUT( \
     KC_ESC         , KC_1          , KC_2          , KC_3          , KC_4          , KC_5          ,\
     KC_TAB         , KC_Q          , KC_W          , KC_E          , KC_R          , KC_T          ,\
     KC_LSFT        , KC_A          , KC_S          , KC_D          , KC_F          , KC_G          ,\
     KC_LSFT        , KC_Z          , KC_X          , KC_C          , KC_V          , KC_B          ,\
     KC_LCTL        , KC_NO                                         , KC_SPC       , KC_SPC        
  ),
  [_L3] = LAYOUT( \
     KC_ESC         , KC_1          , KC_2          , KC_3          , KC_4          , KC_5          ,\
     KC_TAB         , KC_Q          , KC_W          , KC_E          , KC_R          , KC_T          ,\
     KC_LSFT        , KC_A          , KC_S          , KC_D          , KC_F          , KC_G          ,\
     KC_LSFT        , KC_Z          , KC_X          , KC_C          , KC_V          , KC_B          ,\
     KC_LCTL        , KC_NO                                         , KC_SPC       , KC_SPC        
  )
};

// clang-format on
