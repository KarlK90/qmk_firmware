// Copyright 2023 Stefan Kerkmann (@KarlK90)
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <ch.h>
#include <hal.h>
#include <stdint.h>
#include <stdbool.h>

#include "timer.h"

typedef struct {
    fast_timer_t last_report;
    uint16_t     idle_rate;
} usb_idle_timer_t;

typedef struct {
    usb_idle_timer_t *timers;
    void (*set_idle)(usb_idle_timer_t *, uint8_t, uint8_t);
    uint8_t (*get_idle)(usb_idle_timer_t *, uint8_t);
    bool (*idle_timer_elasped)(usb_idle_timer_t *, uint8_t);
} usb_idle_timers_t;

void    usb_idle_task(void);
void    usb_set_idle_rate(usb_idle_timer_t *timers, uint8_t report_id, uint8_t idle_rate);
uint8_t usb_get_idle_rate(usb_idle_timer_t *timers, uint8_t report_id);
bool    usb_idle_timer_elapsed(usb_idle_timer_t *timers, uint8_t report_id);
void    usb_shared_set_idle_rate(usb_idle_timer_t *timers, uint8_t report_id, uint8_t idle_rate);
uint8_t usb_shared_get_idle_rate(usb_idle_timer_t *timers, uint8_t report_id);
bool    usb_shared_idle_timer_elapsed(usb_idle_timer_t *timers, uint8_t report_id);

#define QMK_HID_IDLE_TIMERS(size, _get_idle, _set_idle, _idle_timer_elasped) \
    &((usb_idle_timers_t){                                                   \
        .timers             = (_Alignas(4) usb_idle_timer_t[size]){{0}},     \
        .get_idle           = _get_idle,                                     \
        .set_idle           = _set_idle,                                     \
        .idle_timer_elasped = _idle_timer_elasped,                           \
    })

#define QMK_HID_DEFAULT_IDLE_TIMERS QMK_HID_IDLE_TIMERS(1, &usb_get_idle_rate, &usb_set_idle_rate, &usb_idle_timer_elapsed)

bool usb_get_idle_cb(USBDriver *driver);
bool usb_set_idle_cb(USBDriver *driver);
