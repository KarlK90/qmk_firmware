// Copyright 2023 Stefan Kerkmann (@KarlK90)
// Copyright 2020 Ryan (@fauxpark)
// Copyright 2020 Joel Challis (@zvecr)
// Copyright 2018 James Laird-Wah
// Copyright 2016 Fredizzimo
// Copyright 2016 Giovanni Di Sirio
// SPDX-License-Identifier: GPL-3.0-or-later OR Apache-2.0

#pragma once

#include <ch.h>
#include <hal.h>

#include "usb_device_state.h"
#include "usb_descriptor.h"
#include "usb_driver.h"

#if !defined(USB_DEFAULT_BUFFER_CAPACITY)
#    define USB_DEFAULT_BUFFER_CAPACITY 4
#endif

#if !defined(KEYBOARD_IN_CAPACITY)
#    define KEYBOARD_IN_CAPACITY USB_DEFAULT_BUFFER_CAPACITY
#endif
#if !defined(SHARED_IN_CAPACITY)
#    define SHARED_IN_CAPACITY USB_DEFAULT_BUFFER_CAPACITY
#endif
#if !defined(MOUSE_IN_CAPACITY)
#    define MOUSE_IN_CAPACITY USB_DEFAULT_BUFFER_CAPACITY
#endif

#if !defined(JOYSTICK_IN_CAPACITY)
#    define JOYSTICK_IN_CAPACITY USB_DEFAULT_BUFFER_CAPACITY
#endif

#if !defined(DIGITIZER_IN_CAPACITY)
#    define DIGITIZER_IN_CAPACITY USB_DEFAULT_BUFFER_CAPACITY
#endif

#if !defined(CONSOLE_IN_CAPACITY)
#    define CONSOLE_IN_CAPACITY USB_DEFAULT_BUFFER_CAPACITY
#endif

#if !defined(CONSOLE_OUT_CAPACITY)
#    define CONSOLE_OUT_CAPACITY USB_DEFAULT_BUFFER_CAPACITY
#endif

#if !defined(RAW_IN_CAPACITY)
#    define RAW_IN_CAPACITY USB_DEFAULT_BUFFER_CAPACITY
#endif

#if !defined(RAW_OUT_CAPACITY)
#    define RAW_OUT_CAPACITY USB_DEFAULT_BUFFER_CAPACITY
#endif

#if !defined(MIDI_STREAM_IN_CAPACITY)
#    define MIDI_STREAM_IN_CAPACITY USB_DEFAULT_BUFFER_CAPACITY
#endif

#if !defined(MIDI_STREAM_OUT_CAPACITY)
#    define MIDI_STREAM_OUT_CAPACITY USB_DEFAULT_BUFFER_CAPACITY
#endif

#if !defined(CDC_IN_CAPACITY)
#    define CDC_IN_CAPACITY USB_DEFAULT_BUFFER_CAPACITY
#endif

#if !defined(CDC_OUT_CAPACITY)
#    define CDC_OUT_CAPACITY USB_DEFAULT_BUFFER_CAPACITY
#endif

#define CDC_SIGNALING_DUMMY_CAPACITY 1

typedef enum {
#if defined(SHARED_EP_ENABLE)
    USB_ENDPOINT_IN_SHARED,
#endif

#if !defined(KEYBOARD_SHARED_EP)
    USB_ENDPOINT_IN_KEYBOARD,
#endif

#if defined(MOUSE_ENABLE) && !defined(MOUSE_SHARED_EP)
    USB_ENDPOINT_IN_MOUSE,
#endif

#if defined(JOYSTICK_ENABLE) && !defined(JOYSTICK_SHARED_EP)
    USB_ENDPOINT_IN_JOYSTICK,
#endif

#if defined(DIGITIZER_ENABLE) && !defined(DIGITIZER_SHARED_EP)
    USB_ENDPOINT_IN_DIGITIZER,
#endif

#if defined(CONSOLE_ENABLE)
    USB_ENDPOINT_IN_CONSOLE,
#endif

#if defined(RAW_ENABLE)
    USB_ENDPOINT_IN_RAW,
#endif

#if defined(MIDI_ENABLE)
    USB_ENDPOINT_IN_MIDI,
#endif

#if defined(VIRTSER_ENABLE)
    USB_ENDPOINT_IN_CDC_DATA,
    USB_ENDPOINT_IN_CDC_SIGNALING,
#endif
    USB_ENDPOINT_IN_COUNT,
/* All non shared endpoints have to be consequtive numbers starting from 0, so
 * that they can be used as array indices. The shared endpoints all point to
 * the same endpoint so they have to be defined last to not reset the enum
 * counter. */
#if defined(SHARED_EP_ENABLE)
#    if defined(KEYBOARD_SHARED_EP)
    USB_ENDPOINT_IN_KEYBOARD = USB_ENDPOINT_IN_SHARED,
#    endif
#    if defined(MOUSE_SHARED_EP)
    USB_ENDPOINT_IN_MOUSE = USB_ENDPOINT_IN_SHARED,
#    endif
#    if defined(JOYSTICK_SHARED_EP)
    USB_ENDPOINT_IN_JOYSTICK = USB_ENDPOINT_IN_SHARED,
#    endif
#    if defined(DIGITIZER_SHARED_EP)
    USB_ENDPOINT_IN_DIGITIZER = USB_ENDPOINT_IN_SHARED,
#    endif
#endif
} usb_endpoint_in_lut_t;

typedef enum {
#if defined(CONSOLE_ENABLE)
    USB_ENDPOINT_OUT_CONSOLE,
#endif
#if defined(RAW_ENABLE)
    USB_ENDPOINT_OUT_RAW,
#endif
#if defined(MIDI_ENABLE)
    USB_ENDPOINT_OUT_MIDI,
#endif
#if defined(VIRTSER_ENABLE)
    USB_ENDPOINT_OUT_CDC_DATA,
#endif
    USB_ENDPOINT_OUT_COUNT,
} usb_endpoint_out_lut_t;

/* -------------------------
 * General USB driver header
 * -------------------------
 */

/* The USB driver to use */
#ifndef USB_DRIVER
#    define USB_DRIVER USBD1
#endif // USB_DRIVER

/* Initialize the USB driver and bus */
void init_usb_driver(USBDriver *usbp);

/* Restart the USB driver and bus */
void restart_usb_driver(USBDriver *usbp);

/* ---------------
 * USB Event queue
 * ---------------
 */

/* Initialisation of the FIFO */
void usb_event_queue_init(void);

/* Task to dequeue and execute any handlers for the USB events on the main thread */
void usb_event_queue_task(void);

/* ----------------
 * USB Idle handler
 * ----------------
 */

 void usb_idle_task(void);

/* --------------
 * Console header
 * --------------
 */

#ifdef CONSOLE_ENABLE

/* Putchar over the USB console */
int8_t sendchar(uint8_t c);

/* Flush output (send everything immediately) */
void console_flush_output(void);

#endif /* CONSOLE_ENABLE */
