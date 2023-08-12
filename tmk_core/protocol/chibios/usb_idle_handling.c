// Copyright 2023 Stefan Kerkmann (@KarlK90)
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string.h>
#include <stdint.h>

#include "usb_report_handling.h"
#include "usb_idle_handling.h"
#include "usb_endpoints.h"
#include "usb_driver.h"
#include "usb_main.h"
#include "report.h"

extern usb_endpoint_in_t usb_endpoints_in[USB_ENDPOINT_IN_COUNT];

void usb_set_idle_rate(usb_idle_timer_t *timers, uint8_t report_id, uint8_t idle_rate) {
    (void)report_id;
    timers[0].idle_rate = idle_rate * 4;
}

uint8_t usb_get_idle_rate(usb_idle_timer_t *timers, uint8_t report_id) {
    (void)report_id;
    return timers[0].idle_rate / 4;
}

bool usb_idle_timer_elapsed(usb_idle_timer_t *timers, uint8_t report_id) {
    (void)report_id;

    chSysLock();
    uint16_t idle_rate = timers[0].idle_rate;
    chSysUnlock();

    if (idle_rate == 0) {
        return false;
    }

    bool elapsed = timer_elapsed_fast(timers[0].last_report) >= idle_rate;

    if (elapsed) {
        timers[0].last_report = timer_read_fast();
    }

    return elapsed;
}

void usb_shared_set_idle_rate(usb_idle_timer_t *timers, uint8_t report_id, uint8_t idle_rate) {
    if (report_id == 0 || report_id > REPORT_ID_COUNT) {
        return;
    }
    timers[report_id - 1].idle_rate = idle_rate * 4;
}

uint8_t usb_shared_get_idle_rate(usb_idle_timer_t *timers, uint8_t report_id) {
    if (report_id == 0 || report_id > REPORT_ID_COUNT) {
        return 0;
    }
    return timers[report_id - 1].idle_rate / 4;
}

bool usb_shared_idle_timer_elapsed(usb_idle_timer_t *timers, uint8_t report_id) {
    if (report_id == 0 || report_id > REPORT_ID_COUNT) {
        return false;
    }

    chSysLock();
    uint16_t idle_rate = timers[report_id - 1].idle_rate;
    chSysUnlock();

    if (idle_rate == 0) {
        return false;
    }

    bool elapsed = timer_elapsed_fast(timers[report_id - 1].last_report) >= idle_rate;

    if (elapsed) {
        timers[report_id - 1].last_report = timer_read_fast();
    }

    return elapsed;
}

void usb_idle_task(void) {
    static usb_fs_report_t report;

    for (int i = 0; i < USB_ENDPOINT_IN_COUNT; i++) {
        usb_report_storage_t *report_storage = usb_endpoints_in[i].report_storage;
        usb_idle_timers_t    *idle_timers    = usb_endpoints_in[i].idle_timers;

        if (report_storage == NULL || idle_timers == NULL) {
            continue;
        }

#if defined(SHARED_EP_ENABLE)
        if (i == USB_ENDPOINT_IN_SHARED) {
            for (int j = 1; j <= REPORT_ID_COUNT; j++) {
                if (idle_timers->idle_timer_elasped(idle_timers->timers, j)) {
                    chSysLock();
                    report_storage->get_report(report_storage->reports, j, &report);
                    chSysUnlock();
                    send_report(i, &report.data, report.length);
                }
            }
            continue;
        }
#endif
        if (idle_timers->idle_timer_elasped(idle_timers->timers, 0)) {
            chSysLock();
            report_storage->get_report(report_storage->reports, 0, &report);
            chSysUnlock();
            send_report(i, &report.data, report.length);
        }
    }
}

bool usb_get_idle_cb(USBDriver *driver) {
    usb_idle_timers_t *idle_timers;
    static uint8_t _Alignas(4) idle_rate;

    switch (driver->setup[4]) { /* LSB(wIndex) (check MSB==0?) */
#ifndef KEYBOARD_SHARED_EP
        case KEYBOARD_INTERFACE:
            idle_timers = usb_endpoints_in[USB_ENDPOINT_IN_KEYBOARD].idle_timers;
            idle_rate   = idle_timers->get_idle(idle_timers->timers, 0);
            break;
#endif
        case SHARED_INTERFACE:
            uint8_t report_id = usbp->setup[2];
            idle_timers       = usb_endpoints_in[USB_ENDPOINT_IN_SHARED].idle_timers;
            idle_rate         = idle_timers->get_idle(idle_timers->timers, report_id);
            break;
#if defined(MOUSE_ENABLE) && !defined(MOUSE_SHARED_EP)
        case MOUSE_INTERFACE:
            idle_timers = usb_endpoints_in[USB_ENDPOINT_IN_MOUSE].idle_timers;
            idle_rate   = idle_timers->get_idle(idle_timers->timers, 0);
            break;
#endif
#if defined(JOYSTICK_ENABLE) && !defined(JOYSTICK_SHARED_EP)
        case JOYSTICK_INTERFACE:
            idle_timers = usb_endpoints_in[USB_ENDPOINT_IN_JOYSTICK].idle_timers;
            idle_rate   = idle_timers->get_idle(idle_timers->timers, 0);
            break;
#endif
#if defined(DIGITIZER_ENABLE) && !defined(DIGITIZER_SHARED_EP)
        case DIGITIZER_INTERFACE:
            idle_timers = usb_endpoints_in[USB_ENDPOINT_IN_DIGITIZER].idle_timers;
            idle_rate   = idle_timers->get_idle(idle_timers->timers, 0);
            break;
#endif
#if defined(CONSOLE_ENABLE)
        case CONSOLE_INTERFACE:
            idle_timers = usb_endpoints_in[USB_ENDPOINT_IN_CONSOLE].idle_timers;
            idle_rate   = idle_timers->get_idle(idle_timers->timers, 0);
            break;
#endif
#if defined(RAW_ENABLE)
        case RAW_INTERFACE:
            idle_timers = usb_endpoints_in[USB_ENDPOINT_IN_RAW].idle_timers;
            idle_rate   = idle_timers->get_idle(idle_timers->timers, 0);
            break;
#endif
        default:
            // Unknown interface, abort
            return false;
    }

    usbSetupTransfer(driver, &idle_rate, 1, NULL);

    return true;
}

bool usb_set_idle_cb(USBDriver *driver) {
    usb_idle_timers_t *idle_timers;
    uint8_t            idle_rate = driver->setup[3];

    switch (driver->setup[4]) { /* LSB(wIndex) (check MSB==0?) */
#ifndef KEYBOARD_SHARED_EP
        case KEYBOARD_INTERFACE:
            idle_timers = usb_endpoints_in[USB_ENDPOINT_IN_KEYBOARD].idle_timers;
            idle_timers->set_idle(idle_timers->timers, 0, idle_rate);
            break;
#endif
#if defined(SHARED_EP_ENABLE)
        case SHARED_INTERFACE:
            uint8_t report_id = usbp->setup[2];
            idle_timers       = usb_endpoints_in[USB_ENDPOINT_IN_SHARED].idle_timers;
            idle_timers->set_idle(idle_timers->timers, report_id, idle_rate);
            break;
#endif
#if defined(MOUSE_ENABLE) && !defined(MOUSE_SHARED_EP)
        case MOUSE_INTERFACE:
            idle_timers = usb_endpoints_in[USB_ENDPOINT_IN_MOUSE].idle_timers;
            idle_timers->set_idle(idle_timers->timers, 0, idle_rate);
            break;
#endif
#if defined(JOYSTICK_ENABLE) && !defined(JOYSTICK_SHARED_EP)
        case JOYSTICK_INTERFACE:
            idle_timers = usb_endpoints_in[USB_ENDPOINT_IN_JOYSTICK].idle_timers;
            idle_timers->set_idle(idle_timers->timers, 0, idle_rate);
            break;
#endif
#if defined(DIGITIZER_ENABLE) && !defined(DIGITIZER_SHARED_EP)
        case DIGITIZER_INTERFACE:
            idle_timers = usb_endpoints_in[USB_ENDPOINT_IN_DIGITIZER].idle_timers;
            idle_timers->set_idle(idle_timers->timers, 0, idle_rate);
            break;
#endif
#if defined(CONSOLE_ENABLE)
        case CONSOLE_INTERFACE:
            idle_timers = usb_endpoints_in[USB_ENDPOINT_IN_CONSOLE].idle_timers;
            idle_timers->set_idle(idle_timers->timers, 0, idle_rate);
            break;
#endif
#if defined(RAW_ENABLE)
        case RAW_INTERFACE:
            idle_timers = usb_endpoints_in[USB_ENDPOINT_IN_RAW].idle_timers;
            idle_timers->set_idle(idle_timers->timers, 0, idle_rate);
            break;
#endif
        default:
            // Unknown interface, abort
            return false;
    }

    usbSetupTransfer(driver, NULL, 0, NULL);

    return true;
}
