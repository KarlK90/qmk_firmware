// Copyright 2021 Christoph Rehmann (crehmann)
// SPDX-License-Identifier: GPL-2.0-or-later

#include "haptic_utils.h"

#ifdef HAPTIC_ENABLE
#include "drivers/haptic/drv2605l.h"
#endif

#ifdef HAPTIC_ENABLE
bool get_haptic_enabled_key(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case KC_BTN1 ... KC_BTN5:
            return true;
            break;
    }

    return false;
}
#endif

void process_layer_pulse(layer_state_t state) {
#ifdef HAPTIC_ENABLE
    switch (get_highest_layer(state)) {
        case 1:
            drv2605l_pulse(soft_bump);
            break;
        case 2:
            drv2605l_pulse(sh_dblsharp_tick);
            break;
        case 3:
            drv2605l_pulse(lg_dblclick_str);
            break;
        case 4:
            drv2605l_pulse(soft_bump);
            break;
        case 5:
            drv2605l_pulse(pulsing_sharp);
            break;
    }
#endif
}
