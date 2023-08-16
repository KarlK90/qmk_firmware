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

extern usb_endpoint_in_t     usb_endpoints_in[USB_ENDPOINT_IN_COUNT];
extern usb_endpoint_in_lut_t usb_endpoint_interface_lut[TOTAL_INTERFACES];

static bool run_idle_task = false;

void usb_set_idle_rate(usb_idle_timer_t *timers, uint8_t report_id, uint8_t idle_rate) {
    (void)report_id;

    timers[0].idle_rate = idle_rate * 4;

    run_idle_task |= idle_rate != 0;
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

    fast_timer_t now = timer_read_fast();

    bool elapsed = TIMER_DIFF_FAST(now, timers[0].last_report) >= idle_rate;

    if (elapsed) {
        timers[0].last_report = now;
    }

    return elapsed;
}

void usb_shared_set_idle_rate(usb_idle_timer_t *timers, uint8_t report_id, uint8_t idle_rate) {
    if (report_id > REPORT_ID_COUNT) {
        return;
    }

    // Set idle rate for all reports if report_id is 0
    if (report_id == 0) {
        for (int id = 1; id <= REPORT_ID_COUNT; id++) {
            usb_shared_set_idle_rate(timers, id, idle_rate);
        }
        return;
    }

    timers[report_id - 1].idle_rate = idle_rate * 4;

    run_idle_task |= idle_rate != 0;
}

uint8_t usb_shared_get_idle_rate(usb_idle_timer_t *timers, uint8_t report_id) {
    if (report_id > REPORT_ID_COUNT) {
        return 0;
    }

    // Return idle rate for first report if report_id is 0, as we assume that
    // all reports have the same idle rate in this case.
    if (report_id == 0) {
        report_id = 1;
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

    fast_timer_t now = timer_read_fast();

    bool elapsed = TIMER_DIFF_FAST(now, timers[report_id - 1].last_report) >= idle_rate;

    if (elapsed) {
        timers[report_id - 1].last_report = now;
    }

    return elapsed;
}

void usb_idle_task(void) {
    if (!run_idle_task) {
        return;
    }

    static usb_fs_report_t report;
    bool                   non_zero_idle_rate_found = false;

    for (int ep = 0; ep < USB_ENDPOINT_IN_COUNT; ep++) {
        usb_report_storage_t *report_storage = usb_endpoints_in[ep].report_storage;
        usb_idle_timers_t    *idle_timers    = usb_endpoints_in[ep].idle_timers;

        if (report_storage == NULL || idle_timers == NULL) {
            continue;
        }

#if defined(SHARED_EP_ENABLE)
        if (ep == USB_ENDPOINT_IN_SHARED) {
            for (int report_id = 1; report_id <= REPORT_ID_COUNT; report_id++) {
                chSysLock();
                non_zero_idle_rate_found |= idle_timers->get_idle(idle_timers->timers, report_id) != 0;
                chSysUnlock();

                if (idle_timers->idle_timer_elasped(idle_timers->timers, report_id)) {
                    chSysLock();
                    report_storage->get_report(report_storage->reports, report_id, &report);
                    chSysUnlock();
                    send_report(ep, &report.data, report.length);
                }
            }
            continue;
        }
#endif

        chSysLock();
        non_zero_idle_rate_found |= idle_timers->get_idle(idle_timers->timers, 0) != 0;
        chSysUnlock();

        if (idle_timers->idle_timer_elasped(idle_timers->timers, 0)) {
            chSysLock();
            report_storage->get_report(report_storage->reports, 0, &report);
            chSysUnlock();
            send_report(ep, &report.data, report.length);
        }
    }

    run_idle_task = non_zero_idle_rate_found;
}

bool usb_get_idle_cb(USBDriver *driver) {
    static uint8_t _Alignas(4) idle_rate;

    uint8_t interface = driver->setup[4];
    uint8_t report_id = driver->setup[2];

    if (!IS_VALID_INTERFACE(interface) || !IS_VALID_REPORT_ID(report_id)) {
        return false;
    }

    usb_endpoint_in_lut_t ep = usb_endpoint_interface_lut[interface];

    if (!IS_VALID_USB_ENDPOINT_IN_LUT(ep)) {
        return false;
    }

    usb_idle_timers_t *idle_timers = usb_endpoints_in[ep].idle_timers;

    if (idle_timers == NULL) {
        return false;
    }

    idle_rate = idle_timers->get_idle(idle_timers->timers, report_id);

    usbSetupTransfer(driver, &idle_rate, 1, NULL);

    return true;
}

bool usb_set_idle_cb(USBDriver *driver) {
    uint8_t idle_rate = driver->setup[3];
    uint8_t interface = driver->setup[4];
    uint8_t report_id = driver->setup[2];

    if (!IS_VALID_INTERFACE(interface) || !IS_VALID_REPORT_ID(report_id)) {
        return false;
    }

    usb_endpoint_in_lut_t ep = usb_endpoint_interface_lut[interface];

    if (!IS_VALID_USB_ENDPOINT_IN_LUT(ep)) {
        return false;
    }

    usb_idle_timers_t *idle_timers = usb_endpoints_in[ep].idle_timers;

    if (idle_timers == NULL) {
        return false;
    }

    idle_timers->set_idle(idle_timers->timers, report_id, idle_rate);

    usbSetupTransfer(driver, NULL, 0, NULL);

    return true;
}
