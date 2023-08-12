// Copyright 2023 Stefan Kerkmann (@KarlK90)
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "usb_report_handling.h"
#include "usb_endpoints.h"
#include "usb_driver.h"
#include "report.h"

extern usb_endpoint_in_t usb_endpoints_in[USB_ENDPOINT_IN_COUNT];

void usb_set_report(usb_fs_report_t *reports, const uint8_t *data, size_t length) {
    reports[0].length = length;
    memcpy(&reports[0].data, data, length);
}

void usb_get_report(usb_fs_report_t *reports, uint8_t report_id, usb_fs_report_t *report) {
    (void)report_id;
    report->length = reports[0].length;
    memcpy(&report->data, &reports[0].data, report->length);
}

void usb_shared_set_report(usb_fs_report_t *reports, const uint8_t *data, size_t length) {
    uint8_t report_id = data[0];
    if (report_id == 0 || report_id > REPORT_ID_COUNT) {
        return;
    }
    reports[report_id - 1].length = length;
    memcpy(&reports[report_id - 1].data, data, length);
}

void usb_shared_get_report(usb_fs_report_t *reports, uint8_t report_id, usb_fs_report_t *report) {
    if (report_id == 0 || report_id > REPORT_ID_COUNT) {
        return;
    }
    report->length = reports[report_id - 1].length;
    memcpy(&report->data, &reports[report_id - 1].data, report->length);
}

bool usb_get_report_cb(USBDriver *driver) {
    static usb_fs_report_t report;
    usb_report_storage_t  *report_storage;
    switch (driver->setup[4]) {
#ifndef KEYBOARD_SHARED_EP
        case KEYBOARD_INTERFACE:
            report_storage = usb_endpoints_in[USB_ENDPOINT_IN_KEYBOARD].report_storage;
            report_storage->get_report(report_storage->reports, 0, &report);
            break;
#endif
#ifdef SHARED_EP_ENABLE
        case SHARED_INTERFACE:
            uint8_t report_id = driver->setup[2];
            report_storage    = usb_endpoints_in[USB_ENDPOINT_IN_SHARED].report_storage;
            report_storage->get_report(report_storage->reports, report_id, &report);
            break;
#endif /* SHARED_EP_ENABLE */
#if defined(MOUSE_ENABLE) && !defined(MOUSE_SHARED_EP)
        case MOUSE_INTERFACE:
            report_storage = usb_endpoints_in[USB_ENDPOINT_IN_MOUSE].report_storage;
            report_storage->get_report(report_storage->reports, 0, &report);
            break;
#endif
#if defined(JOYSTICK_ENABLE) && !defined(JOYSTICK_SHARED_EP)
        case JOYSTICK_INTERFACE:
            report_storage = usb_endpoints_in[USB_ENDPOINT_IN_JOYSTICK].report_storage;
            report_storage->get_report(report_storage->reports, 0, &report);
            break;
#endif
#if defined(DIGITIZER_ENABLE) && !defined(DIGITIZER_SHARED_EP)
        case DIGITIZER_INTERFACE:
            report_storage = usb_endpoints_in[USB_ENDPOINT_IN_DIGITIZER].report_storage;
            report_storage->get_report(report_storage->reports, 0, &report);
            break;
#endif
#if defined(CONSOLE_ENABLE)
        case CONSOLE_INTERFACE:
            report_storage = usb_endpoints_in[USB_ENDPOINT_IN_CONSOLE].report_storage;
            report_storage->get_report(report_storage->reports, 0, &report);
            break;
#endif
#if defined(RAW_ENABLE)
        case RAW_INTERFACE:
            report_storage = usb_endpoints_in[USB_ENDPOINT_IN_RAW].report_storage;
            report_storage->get_report(report_storage->reports, 0, &report);
            break;
#endif
        default:
            // Unknown interface
            return false;
    }

    usbSetupTransfer(driver, (uint8_t *)report.data, report.length, NULL);

    return true;
}
