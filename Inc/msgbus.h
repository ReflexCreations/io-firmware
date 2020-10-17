#ifndef __MESSAGE_BUS_H
#define __MESSAGE_BUS_H

#include "stm32f3xx.h"
#include "uart.h"

#define MAX_REQUEST_DATA_BYTES (64U)
#define MAX_RESPONSE_DATA_BYTES (64U)

typedef struct {
    // Which port this request is being made over
    ComportId comport_id;

    // Command byte being sent first
    uint8_t request_command;

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

typedef struct {
    // Which port this response came in from
    ComportId comport_id;

    // What command was initially sent to get this in response
    // Copied from the request struct that caused this response
    uint8_t request_command;

    // Pointer to data sent in the response
    uint8_t * data;

    // Number of bytes contained in the response; should be at least 1.
    // This number is copied from the Request struct that caused this response
    uint8_t data_length;
} Response;

typedef enum {
    // Not been given a request to send
    Status_Idle,

    // Been given a request to send, but the port is not currently active
    Status_Queued,

    // DMA for sending the command byte has started
    Status_Sending_Command,

    // DMA for sending the data has started
    Status_Sending_Data,

    // DMA for receiving data has been started
    Status_Receiving,

    // Has finished processing a message.
    // Gets reset to idle when port deactivates.
    Status_Done
} PortStatus;

typedef struct {
    // Which port this is about
    ComportId comport_id;

    // Current status (as in a state machine) of this port
    // See PortStatus for details
    PortStatus status;
    // Whether this state is currently "selected" - as in, it's its turn
    // to do comport things. If not selected, data can be queued for it is.
    uint8_t selected;

    // Request this port is currently working on, or queued for
    // being worked on when selected/
    Request current_request;

    // Response the current request is working with, only relevant if
    // current_request.response_len > 0
    Response current_response;

    // Interrupt flags to be processed
    uint8_t interrupt_flags;
} PortState;

typedef struct {
    PortState * first;
    PortState * second;
} PortPair;

// Sets the message bus up for use
void msgbus_init();

// To be called every main loop iteration, for processing interrupt flags
// not during interrupt execution
void msgbus_process_flags();

// Asks for a request to be sent through the mentioned port.
// If this port is currently busy with a request, the request will be
// discarded.
void msgbus_send_request(Request request);

// Checks whether there is a pending response in the queue. You must check this and
// and only use msgbus_get_pending_response if it returns true (non-0)
uint8_t msgbus_have_pending_response();

// Returns the oldest response from the response queue. You must check
// msgbus_have_pending_response() before using this method. If there is no pending
// response, Error_Handler() will be invoked.
Response * msgbus_get_pending_response();

void msgbus_switch_ports_if_done();

#endif