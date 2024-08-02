// Copyright 2025 Stefan Kerkmann (@karlk90)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

// clang-format off
const uint16_t keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [0] = LAYOUT(
    KC_1,    KC_2,  KC_3,   KC_4,  KC_5,    KC_6,
    KC_TAB,        KC_Q,    KC_W,  KC_E,    KC_R,  KC_T,   KC_Y,  KC_U,    KC_I,    KC_O,   KC_P,    KC_BSPC,
    LT(1, KC_ESC), KC_A,    KC_S,  KC_D,    KC_F,  KC_G,   KC_H,  KC_J,    KC_K,    KC_L,   KC_SCLN, LT(1, KC_QUOT),
    KC_LSFT,       KC_Z,    KC_X,  KC_C,    KC_V,  KC_B,   KC_N,  KC_M,    KC_COMM, KC_DOT, KC_SLSH, LT(2, KC_ENT),
    KC_LCTL,       MO(2),   KC_LGUI, XXXXXXX, XXXXXXX, KC_SPC, XXXXXXX, KC_RSFT, KC_RALT, TG(3)
  ),
  [1] = LAYOUT(
    _______, _______, _______, _______, _______, _______,
    KC_GRV,  KC_EXLM, KC_AT,   KC_HASH, KC_DLR,  KC_PERC, KC_CIRC, KC_AMPR, KC_ASTR, KC_LPRN, KC_RPRN,  KC_DEL,
    _______, KC_BSLS, KC_QUOT, KC_MINS, KC_EQL,  KC_LBRC, KC_RBRC, KC_DOWN, KC_UP,   KC_LEFT, KC_RIGHT, _______,
    _______, KC_ESC,  _______, KC_PSCR, _______, _______, _______, KC_MSTP, KC_MPLY, KC_MPRV, KC_MNXT , _______,
    _______, _______, _______, XXXXXXX, XXXXXXX,                   _______, XXXXXXX, _______, QK_BOOT , QK_RBT
  ),
  [2] = LAYOUT(
    _______, _______, _______, _______, _______, _______,
    KC_TILD, KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,     _______,
    KC_ESC,  KC_PIPE, KC_DQT,  KC_UNDS, KC_PLUS, KC_LCBR, KC_RCBR, KC_4,    KC_5,    KC_6,    KC_VOLU,  _______,
    _______, _______, _______, _______, _______, KC_0,    KC_1,    KC_2,    KC_3,    KC_VOLD, _______,  _______,
    _______, _______, XXXXXXX, XXXXXXX, _______,                   XXXXXXX,   _______, _______, _______,  _______
  ),
  [3] = LAYOUT(
    RM_TOGG, RM_NEXT, RM_PREV, RM_VALU, RM_VALD, _______,
    _______, RM_SPDU, RM_SPDD, RM_HUEU, RM_HUED, RM_VALU, RM_VALD, KC_F1,   KC_F2,   KC_F3,   KC_F4,    _______,
    KC_ESC,  _______, _______, _______, _______, _______, _______, KC_F5,   KC_F6,   KC_F7,   KC_F8,    _______,
    KC_LSFT, _______, _______, _______, _______, _______, _______, KC_F9,   KC_F10,  KC_F11,  KC_F12,   _______,
    _______, KC_LALT, _______, XXXXXXX, XXXXXXX,                     _______, XXXXXXX,   _______, _______,  _______
  )
};
// clang-format on

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    if (layer_state_is(1)) {
        RGB_MATRIX_INDICATOR_SET_COLOR(2, 60, 15, 15);
    } else {
        RGB_MATRIX_INDICATOR_SET_COLOR(2, 0, 0, 0);
    }
    if (layer_state_is(2)) {
        RGB_MATRIX_INDICATOR_SET_COLOR(1, 15, 60, 15);
    } else {
        RGB_MATRIX_INDICATOR_SET_COLOR(1, 0, 0, 0);
    }
    if (layer_state_is(3)) {
        RGB_MATRIX_INDICATOR_SET_COLOR(0, 15, 15, 60);
    } else {
        RGB_MATRIX_INDICATOR_SET_COLOR(0, 0, 0, 0);
    }

    return true;
}
