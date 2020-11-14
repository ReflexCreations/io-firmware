#include "hid_device.h"
#include "tusb_hid.h"

#define PACKET_SIZE (64U)

static uint8_t usb_buffer[PACKET_SIZE];
static bool have_packet = false;

uint8_t const report_descriptor[] = {
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


// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(
    uint8_t report_id,
    hid_report_type_t report_type,
    uint8_t * buffer,
    uint16_t reqlen
) {
    uint16_t report_length = 
        sizeof(report_descriptor) / sizeof(report_descriptor[0]);

    for (uint16_t i = 0; i < report_length; i++) {
        buffer[i] = report_descriptor[i];        
    }

    return report_length;
}


// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(
    uint8_t report_id,
    hid_report_type_t report_type,
    uint8_t const * buffer,
    uint16_t bufsize
) {
    if (report_id == 0 && report_type == 0) {
        for (uint16_t i = 0; i < bufsize; i++) {
            usb_buffer[i] = buffer[i];
        }

        have_packet = true;
    }
}

uint8_t * usb_get_packet() {
    if (!have_packet) return NULL;

    return usb_buffer;
}