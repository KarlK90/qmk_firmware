#pragma once

#include "quantum.h"

// clang-format off

#define LAYOUT( \
     L00, L01, L02, L03, L04, L05, L06,           R01, R02, R03, R04, R05, R06, R07, \
     L10, L11, L12, L13, L14, L15, L16,           R11, R12, R13, R14, R15, R16, R17, \
     L20, L21, L22, L23, L24, L25, L26,           R21, R22, R23, R24, R25, R26, R27, \
     L30, L31, L32, L33, L34, L35, L36, L37, R30, R31, R32, R33, R34, R35, R36, R37, \
                    L43, L44, L45, L46, L47, R40, R41, R42, R43, R44                 \
) \
{ \
     {L00,   L01, L02, L03, L04, L05, L06, KC_NO},\
     {L10,   L11, L12, L13, L14, L15, L16, KC_NO},\
     {L20,   L21, L22, L23, L24, L25, L26, KC_NO},\
     {L30,   L31, L32, L33, L34, L35, L36, L37},\
     {KC_NO, KC_NO, KC_NO, L43, L44, L45, L46, L47},\
     {KC_NO, R01, R02, R03, R04, R05, R06, R07}, \
     {KC_NO, R11, R12, R13, R14, R15, R16, R17}, \
     {KC_NO, R21, R22, R23, R24, R25, R26, R27}, \
     {R30,   R31, R32, R33, R34, R35, R36, R37}, \
     {R40,   R41, R42, R43, R44, KC_NO, KC_NO, KC_NO} \
}

// clang-format on

#define LAYOUT_split_5x8 LAYOUT

void render_display(void);