#ifndef __MESSAGE_BUS_H
#define __MESSAGE_BUS_H

#include "stm32f3xx.h"
#include "request.h"
#include "req_queue.h"
#include "uart.h"
#include "commands.h"

#define MAX_REQUEST_DATA_BYTES (64U)
#define MAX_RESPONSE_DATA_BYTES (64U)

#define MSG_ACKNOWLEGE (0xACU)

typedef struct {
    // Which port this response came in from
    ComportId comport_id;

    // What command was initially sent to get this in response
    // Copied from the request struct that caused this response
    Commands request_command;

    // Pointer to data sent in the response
    uint8_t * data;

    // Number of bytes contained in the response; should be at least 1.
    // This number is copied from the Request struct that caused this response
    uint8_t data_length;
} Response;

typedef enum {
    // Not been given a request to send
    Status_Idle,

    // DMA for sending the command byte has started
    Status_Sending_Command,

    // DMA for receiving the "acknowledge" message active, after which we can 
    // carry on
    Status_Awaiting_Command_Ack,

    // DMA for sending the data has started
    Status_Sending_Data,

    // DMA for receiving the "acknlowedge" message active, after whch we can
    // carry on
    Status_Awaiting_Data_Ack,

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

    // Queue of requests in case we're busy
    RequestQueue req_queue;

    // Response the current request is working with, only relevant if
    // current_request.response_len > 0
    Response current_response;

    // The tick were when we finished sending some data that warrants a
    // response. If we get a certain number of ticks beyond this point without
    // a response, we consider the request to have timed out.
    uint32_t waiting_since;

    // Counts how many times this port has reached a timeout.
    // Only useful when using the debugger at the moment, could be useful
    // for more advanced recovery, perhaps?
    uint32_t timeout_count;

    // Target of the "acknowledge" response byte, should be written to
    // the value of MSG_ACKNOWLEDGE by the uart to indicate receipt of a command
    // or additional data. Once read on this end, should be set back to 0x00.
    uint8_t acknowledged;

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

PortStatus msgbus_port_status(ComportId);

void msgbus_wait_for_idle(ComportId);

void msgbus_switch_ports_if_done();

#endif