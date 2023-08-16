// Copyright 2023 Stefan Kerkmann (@KarlK90)
// SPDX-License-Identifier: GPL-3.0-or-later

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "usb_report_handling.h"
#include "usb_endpoints.h"
#include "usb_driver.h"
#include "report.h"

extern usb_endpoint_in_t     usb_endpoints_in[USB_ENDPOINT_IN_COUNT];
extern usb_endpoint_in_lut_t usb_endpoint_interface_lut[TOTAL_INTERFACES];

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

    uint8_t interface = driver->setup[4];
    uint8_t report_id = driver->setup[2];

    if (!IS_VALID_INTERFACE(interface) || !IS_VALID_REPORT_ID(report_id)) {
        return false;
    }

    usb_endpoint_in_lut_t ep = usb_endpoint_interface_lut[interface];

    if (!IS_VALID_USB_ENDPOINT_IN_LUT(ep)) {
        return false;
    }

    usb_report_storage_t *report_storage = usb_endpoints_in[ep].report_storage;

    if (report_storage == NULL) {
        return false;
    }

    report_storage->get_report(report_storage->reports, report_id, &report);

    usbSetupTransfer(driver, (uint8_t *)report.data, report.length, NULL);

    return true;
}
