/**
  ******************************************************************************
  * @file           : usb_device.c
  * @version        : v2.0_Cube
  * @brief          : This file implements the USB Device
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

#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_customhid.h"
#include "usbd_custom_hid_if.h"
#include "error_handler.h"

// USB Device Core handle declaration. 
USBD_HandleTypeDef hUsbDeviceFS;

void MX_USB_DEVICE_Init(void) {
  // Init Device Library, add supported class and start the library.
  if (USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS) != USBD_OK) {
    Error_Handler(1500);
  }

  if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_CUSTOM_HID) != USBD_OK) {
    Error_Handler(1500);
  }

  if (USBD_CUSTOM_HID_RegisterInterface(
        &hUsbDeviceFS, &USBD_CustomHID_fops_FS) != USBD_OK) {
    Error_Handler(1500);
  }

  if (USBD_Start(&hUsbDeviceFS) != USBD_OK) {
    Error_Handler(1500);
  }
}
