/**
  ******************************************************************************
  * @file           : usbd_custom_hid_if.c
  * @version        : v2.0_Cube
  * @brief          : USB Device Custom HID interface file.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

#include "usbd_custom_hid_if.h"
#include "bool.h"

// Number of buffers we cycle through to ensure we're not reading/writing from
// same one. Not a great solution but don't want to drop frames nor deadlock.
#define RECEIVE_ROTATE_AMOUNT (5U)
#define PACKET_SIZE (64U)

/** Usb HID report descriptor. */
__ALIGN_BEGIN \
static uint8_t CUSTOM_HID_ReportDesc_FS[USBD_CUSTOM_HID_REPORT_DESC_SIZE] \
__ALIGN_END = {
    0x06, 0x00, 0xFF,  // Usage Page (Vendor Defined 0xFF00)
    0x09, 0x01,        // Usage (0x01)
    0xA1, 0x01,        // Collection (Application)
    0x19, 0x01,
    0x29, 0x40,
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x40,        //   Report Count (64)
    0x81, 0x02,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,
                      //   No Null Position)
    0x19, 0x01,
    0x29, 0x40,
    0x75, 0x08,
    0x95, 0x40,        //   Report Count (64)
    0x91, 0x02,        //   Output (Data,Array,Abs,No Wrap,Linear,Preferred State,
                      //   No Null Position,Non-volatile)
    0xC0,              // End Collection
};

extern USBD_HandleTypeDef hUsbDeviceFS;

static int8_t CUSTOM_HID_Init_FS(void);
static int8_t CUSTOM_HID_DeInit_FS(void);
static int8_t CUSTOM_HID_OutEvent_FS(uint8_t event_idx, uint8_t state);

USBD_CUSTOM_HID_ItfTypeDef USBD_CustomHID_fops_FS = {
    CUSTOM_HID_ReportDesc_FS,
    CUSTOM_HID_Init_FS,
    CUSTOM_HID_DeInit_FS,
    CUSTOM_HID_OutEvent_FS
};

uint8_t usb_buffer[PACKET_SIZE * RECEIVE_ROTATE_AMOUNT];
uint32_t packets_received = 0;

volatile uint8_t usb_readable_index = 0;
volatile uint8_t have_new_packet = 0U;

uint8_t * usb_get_packet() {
    if (!have_new_packet) return NULL;
    have_new_packet = 0U;

    return usb_buffer + usb_readable_index * PACKET_SIZE;
}

/**
  * @brief  Initializes the CUSTOM HID media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CUSTOM_HID_Init_FS(void) { return (USBD_OK); }

/**
  * @brief  DeInitializes the CUSTOM HID media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CUSTOM_HID_DeInit_FS(void) { return (USBD_OK); }


// TODO: have two arrays like rep_buf.
// Every time this method gets called, readable_buf swaps to the one we're
// _not_ about to write to. This should hopefully help ensure we're not trying
// to read and write at the same time
//
// Perhaps there should also be some kinda mutex thingimajig?
// That'd rather be an alternative to that, really.
// Could play with debug LEDs to see if we get deadlocked. that could help
// verify whether this function is being called from an interrupt or on
// the main loop.

// Also, if none of that proves helpful, hardcode some data to send over uart
// to see if that portion of the stack works.

static int8_t CUSTOM_HID_OutEvent_FS(uint8_t event_idx, uint8_t state) {
    USBD_CUSTOM_HID_HandleTypeDef *hhid = \
        (USBD_CUSTOM_HID_HandleTypeDef*)hUsbDeviceFS.pClassData;
  
    uint8_t new_index = usb_readable_index + 1;
    if (new_index == RECEIVE_ROTATE_AMOUNT) new_index = 0;

    uint8_t offset = new_index * PACKET_SIZE;
    uint8_t * target = usb_buffer + offset;

    for (uint8_t i = 0; i < PACKET_SIZE; i++){
        target[i] = hhid->Report_buf[i];
    }

    usb_readable_index = new_index;
    have_new_packet = true;
    packets_received++;

    return (USBD_OK);
}
