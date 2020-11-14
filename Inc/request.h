#ifndef __REQUEST_H
#define __REQUEST_H

#include "stm32f3xx.h"
#include "uart.h"
#include "commands.h"
#include "stdbool.h"

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

inline Request request_create(Commands command) {
    Request req;
    req.comport_id = Comport_None;
    req.request_command = command;
    req.send_data = NULL;
    req.send_data_len = 0;
    req.response_data = NULL;
    req.response_len = 0;

    return req;
}

inline uint8_t request_equals(Request req_a, Request req_b) {
    return req_a.comport_id == req_b.comport_id 
        && req_a.request_command == req_b.request_command
        && req_a.send_data == req_b.send_data
        && req_a.send_data_len == req_b.send_data_len
        && req_a.response_data == req_b.response_data
        && req_a.response_len == req_b.response_len;
}

inline uint8_t request_has_data(Request * req) { 
    return req->send_data_len > 0;
}

inline uint8_t request_expects_response(Request * req) {
    return req->response_len > 0;
}

#endif