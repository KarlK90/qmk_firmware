// Copyright 2024 Stefan Kerkmann
// SPDX-License-Identifier: GPL-2.0-or-later

#include <boot/picobin.h>

    .section .image_header, "a"
image_header_base:
    // -- Image start
    .word PICOBIN_BLOCK_MARKER_START
    // -- Image type
    .byte PICOBIN_BLOCK_ITEM_1BS_IMAGE_TYPE
    .byte 0x1
    .hword PICOBIN_IMAGE_TYPE_IMAGE_TYPE_AS_BITS(EXE) | \
           PICOBIN_IMAGE_TYPE_EXE_SECURITY_AS_BITS(S) | \
           PICOBIN_IMAGE_TYPE_EXE_CPU_AS_BITS(ARM)    | \
           PICOBIN_IMAGE_TYPE_EXE_CHIP_AS_BITS(RP2350)
    // -- Vector table
    .byte PICOBIN_BLOCK_ITEM_1BS_VECTOR_TABLE
    .byte 0x2
    .hword 0
    .word __textvectors_base__
    // -- Last item marker
    .byte PICOBIN_BLOCK_ITEM_2BS_LAST
    .hword (image_header_end - image_header_base - 16 ) / 4
    .byte 0
    // -- Last block marker
    .word 0
    // -- Image end
    .word PICOBIN_BLOCK_MARKER_END
image_header_end:
