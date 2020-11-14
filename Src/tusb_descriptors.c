/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "tusb.h"
#include "debug_leds.h"

/* A combination of interfaces must have a unique product id, since PC will save
 * device driver after the first plug.
 * Same VID/PID with different interface e.g MSC (first), then CDC (later) will
 * possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]         HID | MSC | CDC          [LSB]
 */
#define _PID_MAP(itf, n) ((CFG_TUD_##itf) << (n))
#define USB_PID (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | \
                 _PID_MAP(HID, 2) | _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4))

#define USBD_VID 1155
#define USBD_PID_FS 22352


//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = USBD_VID,
    .idProduct          = USBD_PID_FS,
    .bcdDevice          = 0x0200,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const * tud_descriptor_device_cb(void) {
    return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+

uint8_t const desc_hid_report[] = {
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

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_hid_descriptor_report_cb(void) {
  return desc_hid_report;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

uint8_t const desc_configuration[] = {
    // Configuration descriptor ------------------------------------------------
    // Docs:
    // https://www.keil.com/pack/doc/mw/USB/html/_u_s_b__configuration__descriptor.html
    TUD_CONFIG_DESC_LEN, // bLength: Config Descriptor size: 9 bytes
    TUSB_DESC_CONFIGURATION, // bDescriptorType: configuration
    41, // wTotalLength: (low byte) Total size of full descriptor: 41 bytes
    0, // wTotalLength (high byte)
    1, // bNumInterfaces
    1, // bConfigurationValue: Selected configuration id
    0, // iConfiguration: index of string descriptor describing this config
    0xC0, // bmAttributes: 1100 0000 - Self-powered, no remote wakeup
          //               |||\ \\\\ - Reserved, no meaning
          //               ||\- Remote wakeup: off
          //               |\- Self-powered
          //               \- Reserved, must be 1 for historical reasons
    250, // bMaxPower in unites of 2mA; set to the max of 500mA.

    // Interface descriptor, custom HID ---------------------------------------
    // Docs:
    // https://www.keil.com/pack/doc/mw/USB/html/_u_s_b__interface__descriptor.html
    // https://www.usb.org/defined-class-codes
    9, // bLength: Interface descriptor size
    TUSB_DESC_INTERFACE, // bDescriptorType
    0, // bInterfaceNumber: 0-based index
    0, // bAlternateSetting: 0 for not changing settings on the fly
    2, // bNumEndpoints: Number of endpoints in interface: 2:
       // 1. Sensor data: device->host
       // 2. LED data: host->device
    0x03, // bInterfaceClass: Human-Interface-Device (HID)
    0x00, // bInterfaceSubClass: No boot
    0x00, // bInterfaceProtocol: None (not pretending to be mouse or keyboard)
    0,    // iInterface: index of string descriptor for this interface

    // Custom HID Descriptor ---------------------------------------------------
    // Docs:
    // https://www.usb.org/sites/default/files/hid1_11.pdf page 78, E.4
    9, // bLength: HID descriptor size
    TUSB_DESC_CS_DEVICE, // bDescriptorType: HID
    0x11, // bcdHID: Version of the HID specification, binary coded decimal
    0x01, // bcdHID: high byte (version 1.11)
    0x00, // bCountryCode: None / not supported
    0x01, // bNumDescriptors: number of class descriptors to follow
    TUSB_DESC_CS_CONFIGURATION, // bDescriptionType: class specific config
    sizeof(desc_hid_report) / sizeof(desc_hid_report[0]), // wDescriptorLength 
                                                          //for report desc.
    0x00, // wDescriptorLength

    // Endpoint Descriptor -----------------------------------------------------    
    // Docs:
    // https://www.usb.org/sites/default/files/hid1_11.pdf page 78, E.5
    7, // bLength: endpoint descriptor size
    TUSB_DESC_ENDPOINT, // bDescriptorType
    0x81, // 1000 0001 bEndpointAddress
          // |||| \\\\- Endpoint number
          // |\\\- Reserved, forced 0
          // \- Direction: 1 = IN endpoint (device->host)
    0x03, // 0000 0011 bmAttributes
          // |||| ||\\- Transfer type: Interrupt
          // \\\\ \\- Reserved, forced 0
    64,   // wMaxPacketSize: (lobyte) 64 bytes
    0,    // wMaxPacketSize: (hibyte)
    1,    // bInterval: Polling interval expressed in ms

    // Endpoint descriptor -----------------------------------------------------
    // Docs:
    // https://www.usb.org/sites/default/files/hid1_11.pdf page 78, E.5
    7, // bLength: endpoint descriptor size
    TUSB_DESC_ENDPOINT, // bDesccriptorType
    0x01, // 0000 0001 bEndpointAddress
          // |||| \\\\- Endpoint number 
          // |\\\- Reserved, forced 0
          // \- Direction: 0 = OUT endpoint (host->device)
    0x03, // 0000 0011 bmAttributes
          // |||| ||\\- Transfer type: Interrupt
          // \\\\ \\- Reserved, forced 0
    64,   // wMaxPacketSize: (lobyte) 64 bytes
    0,    // wMaxPacketSize: (hibyte)
    1,    // bInterval: Polling interval expressed in ms
};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    (void) index; // for multiple configurations
    return desc_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

char const* string_desc_arr [] = {
    (const char[]) { 0x09, 0x04 }, // 0: Supported language is English (0x0409)
    "Impulse Creations, Ltd.",     // 1: Manufacturer
    "RE:Flex Dance Pad",           // 2: Product
    "123456",                      // 3: Serials, should use chip ID
                                   //    Instead derived at runtime in
                                   //    tud_descriptor_string_cb
};

// Converts a uint32_t into 8 characters representing the hexadecimal
// digits the nibbles (4 bits) it's composed of represent
// but points to an array to copy the resulting characters to.
// This is a uint16_t array because that's tinyusb wants for unicode.
// len specifies the number of nibbles to process. 8 is the maximum.
// Starts reading at the most significant 4 nibbles.
static void uint32_t_to_chars(uint32_t val, uint16_t * buf, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        // Get most significant nibble
        uint32_t char_val = (val >> 28);

        buf[i] = char_val < 0xA 
          ? char_val + '0' // If nibble < 10, char = numeric digit
          : char_val + 'A' - 10; // Otherwise, A-F

        // Shift to next nibble
        val = val << 4;
    }
}

// Storage for string being requested, populated by tud_descriptor_string_cb
static uint16_t _desc_str[32];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long
// enough for transfer to complete
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;

    uint8_t chr_count;

    if (index == 0) {

        // Copy supported language bytes
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
      
    } else if (index == 3) {

        // Derive serial number
        uint32_t serial0, serial1, serial2;

        serial0 = *(uint32_t *) UID_BASE;
        serial1 = *(uint32_t *) (UID_BASE + 4);
        serial2 = *(uint32_t *) (UID_BASE + 8);

        serial0 += serial2;
        uint32_t_to_chars(serial0, _desc_str + 1, 8);
        uint32_t_to_chars(serial1, _desc_str + 9, 4);
        chr_count = 12;

    } else {

        // Convert ASCII string into UTF-16
        // Prevent index out of bounds access
        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) {
            return NULL;
        }

        const char* str = string_desc_arr[index];

        // Cap at max char
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;

        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1+i] = str[i];
        }
    }

    // first byte is length (including header), second byte is string type
    _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2 * chr_count + 2);

    return _desc_str;
}

