// Copyright 2023 Stefan Kerkmann (@KarlK90)
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <ch.h>
#include <hal.h>
#include <stdint.h>

/**
 * @brief Represents a USB Full-speed interrupt report, which is limited to 64
 *        bytes.
 */
typedef struct {
    size_t  length;
    uint8_t data[64];
} usb_fs_report_t;

typedef struct {
    usb_fs_report_t *reports;
    const void (*get_report)(usb_fs_report_t *, uint8_t, usb_fs_report_t *);
    const void (*set_report)(usb_fs_report_t *, const uint8_t *, size_t);
} usb_report_storage_t;

void usb_set_report(usb_fs_report_t *reports, const uint8_t *data, size_t length);
void usb_get_report(usb_fs_report_t *reports, uint8_t report_id, usb_fs_report_t *report);
void usb_shared_set_report(usb_fs_report_t *reports, const uint8_t *data, size_t length);
void usb_shared_get_report(usb_fs_report_t *reports, uint8_t report_id, usb_fs_report_t *report);

#define QMK_USB_REPORT_STORAGE(_get_report, _set_report, _reports...) \
    &((usb_report_storage_t){                                         \
        .reports    = (_Alignas(4) usb_fs_report_t[]){_reports},      \
        .get_report = _get_report,                                    \
        .set_report = _set_report,                                    \
    })

#define QMK_USB_REPORT_STORAGE_DEFAULT(_report_length) QMK_USB_REPORT_STORAGE(&usb_get_report, &usb_set_report, {.length = _report_length})

bool usb_get_report_cb(USBDriver *driver);
