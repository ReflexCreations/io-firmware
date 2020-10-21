#ifndef __REQUEST_H
#define __REQUEST_H

#include "stm32f3xx.h"
#include "uart.h"
#include "commands.h"

typedef struct {
    // Which port this request is being made over
    ComportId comport_id;

    // Command byte being sent first
    Commands request_command;

    // Pointer to start of data to send after command byte (separate transmission)
    // Can be NULL only if send_data_len == 0
    uint8_t * send_data;

    // Number of bytes to send starting from send_data pointer
    uint16_t send_data_len;

    // Pointer to location for response data
    uint8_t * response_data;

    // Number of bytes expected as response
    // Set to 0 to not expect any response after sending request_command + send_data
    uint16_t response_len;
} Request;

#endif