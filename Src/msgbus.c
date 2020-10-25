#include "msgbus.h"
#include "uart.h"
#include "request.h"
#include "req_queue.h"
#include "error_handler.h"

#define RESPONSE_QUEUE_MAX 4

#define INIT_PORT_STATE(state, port_id, is_selected) \
    state.comport_id = port_id; \
    state.status = Status_Idle; \
    state.selected = is_selected; \
    state.current_request = Request_Default; \
    state.current_response = Response_Default; \
    state.interrupt_flags = 0x00; \
    req_queue_init(&state.req_queue);


#define SWITCH_PORTS_IF_DONE() \
    if ((selected_ports.first->status == Status_Done \
            || selected_ports.first->status == Status_Idle) && \
        (selected_ports.second->status == Status_Done \
            || selected_ports.second->status == Status_Idle)) switch_ports()

#define SET_SEND_COMPLETE(__PORTSTATE) \
    __PORTSTATE->interrupt_flags |= (1 << 0); \
    any_interrupt_flags = true;

#define RESET_SEND_COMPLETE(__PORTSTATE) \
    __PORTSTATE->interrupt_flags &= ~(1 << 0)

#define SET_RECEIVE_COMPLETE(__PORTSTATE) \
    __PORTSTATE->interrupt_flags |= (1 << 1); \
    any_interrupt_flags = true;

#define RESET_RECEIVE_COMPLETE(__PORTSTATE) \
    __PORTSTATE->interrupt_flags &= ~(1 << 1)

#define SEND_COMPLETE_IS_SET(__PORTSTATE) \
    (__PORTSTATE.interrupt_flags & 0x01)

#define RECEIVE_COMPLETE_IS_SET(__PORTSTATE) \
    (__PORTSTATE.interrupt_flags & 0x02)

#define PROCESS_FLAGS(__PORTSTATE) \
    if (SEND_COMPLETE_IS_SET(__PORTSTATE)) { \
        process_send_complete(&__PORTSTATE); \
    } \
    \
    if (RECEIVE_COMPLETE_IS_SET(__PORTSTATE)) { \
        process_receive_complete(&__PORTSTATE); \
    }


#define ANY_INTERRUPT_FLAGS (\
    port_state_left.interrupt_flags \
        || port_state_down.interrupt_flags \
        || port_state_up.interrupt_flags \
        || port_state_right.interrupt_flags)

static PortPair selected_ports;
static PortPair unselected_ports;

PortState port_state_left;
PortState port_state_down;
PortState port_state_up;
PortState port_state_right;

static Response * queue_responses[RESPONSE_QUEUE_MAX];
static int8_t queue_front = 0;
static int8_t queue_rear = -1;
static uint8_t queue_count = 0;

// Non-0 if there's any interrupt flags
static uint8_t any_interrupt_flags;

// Init value for this struct
Request Request_Default = {
    Comport_None, // comport_id
    0x00, // request_command
    NULL, // send_data
    0, // send_data_len
    0 // response_len
};

Response Response_Default = { 
    Comport_None, // comport_id
    0x00, // request_command;
    NULL, // data
    0, // data_length
};

static void switch_ports();
static void start_request(Request *);
static PortState * get_port_state(ComportId comport_id);

// Should be given to uart as function pointers
static void uart_on_send_complete(ComportId);
static void uart_on_receive_complete(ComportId);

static void process_send_complete(PortState *);
static void process_receive_complete(PortState *);

static void queue_add(Response *);
static Response * queue_take();


void msgbus_init() {
    INIT_PORT_STATE(port_state_left, Comport_Left, true);
    INIT_PORT_STATE(port_state_up, Comport_Up, true);
    INIT_PORT_STATE(port_state_right, Comport_Right, false);
    INIT_PORT_STATE(port_state_down, Comport_Down, false);

    selected_ports.first = &port_state_left;
    selected_ports.second = &port_state_up;
    unselected_ports.first = &port_state_right;
    unselected_ports.second = &port_state_down;

    uart_connect_port(selected_ports.first->comport_id);
    uart_connect_port(selected_ports.second->comport_id);
    uart_set_on_send_complete_handler(uart_on_send_complete);
    uart_set_on_receive_complete_handler(uart_on_receive_complete);
}

void msgbus_process_flags() {
    if (!ANY_INTERRUPT_FLAGS) {
        SWITCH_PORTS_IF_DONE();
        return;
    }
    // I really ought to use inline functions instead of macros for this
    PROCESS_FLAGS(port_state_left);
    PROCESS_FLAGS(port_state_down);
    PROCESS_FLAGS(port_state_up);
    PROCESS_FLAGS(port_state_right);
    SWITCH_PORTS_IF_DONE();
}

void msgbus_send_request(Request request) {
    PortState * portState = get_port_state(request.comport_id);

    // If no valid port was specified, there's nothing we can do.
    // This is a programming error, so should just brick the program.
    if (portState == NULL) {
        Error_Handler(250);
    }

    // Not Idle? Stick it on the queue
    // Also if port is not selected we'll queue it for later
    if (portState->status != Status_Idle || !portState->selected) {
        req_queue_add(&portState->req_queue, request);
        return;
    }

    // Copy the request over, into the "canonical" location
    // msgbus will use to refer to it from here on
    portState->current_request = request;
    start_request(&portState->current_request);
}

uint8_t msgbus_have_pending_response() {
    return queue_count > 0;
}

Response * msgbus_get_pending_response() {
    return queue_take();
}

void msgbus_switch_ports_if_done() {
    SWITCH_PORTS_IF_DONE();
}

PortStatus msgbus_port_status(ComportId comport_id) {
    PortState * port_state = get_port_state(comport_id);
    if (port_state == NULL) {
        Error_Handler(255);
    }

    return port_state->status;
}

void msgbus_wait_for_idle(ComportId comport_id) {
    PortState * port_state = get_port_state(comport_id);

    if (port_state == NULL) {
        Error_Handler(256);
    }

    while (port_state->status != Status_Idle
            || port_state->req_queue.count > 0) {
        msgbus_process_flags();
    }
}

static void uart_on_send_complete(ComportId comport_id) {
    PortState * port_state = get_port_state(comport_id);
    SET_SEND_COMPLETE(port_state);
}

static void uart_on_receive_complete(ComportId comport_id) {
    PortState * port_state = get_port_state(comport_id);
    SET_RECEIVE_COMPLETE(port_state);
}

static void process_send_complete(PortState * port_state) {
    RESET_SEND_COMPLETE(port_state);

    Request * req = &port_state->current_request;

    switch (port_state->status) {

        case Status_Sending_Command:

            if (req->send_data_len > 0) {

                port_state->status = Status_Sending_Data;

                // Also prepare to receive data now if we're expecting any
                if (req->response_len > 0) {
                    uart_receive(
                        port_state->comport_id,
                        req->response_data,
                        req->response_len
                    );
                }

                // Have additional data to send; begin sending data, update
                // status, then we're done here.
                uart_send(
                    port_state->comport_id,
                    req->send_data,
                    req->send_data_len
                );



                return;

            } else {
                // We'll have already started receiving dma, so only update
                // Status
                if (req->response_len > 0) {
                    port_state->status = Status_Receiving;
                } else {
                    port_state->status = Status_Done;
                    SWITCH_PORTS_IF_DONE();
                }

                return;
            }

        case Status_Sending_Data:
            // Finished sending additional data, now see if we should expect
            // a response right now.
            if (req->response_len > 0) {
                port_state->status = Status_Receiving;
                return;
            } else {
                port_state->status = Status_Done;
                SWITCH_PORTS_IF_DONE();
            }

            break;

        default:
            // If we're getting this callback when not in either of the
            // sending statuses, something is wrong. Panic.
            Error_Handler(251);
            break;
    }
}

static void process_receive_complete(PortState * port_state) {
    RESET_RECEIVE_COMPLETE(port_state);

    Request * req = &port_state->current_request;
    Response * resp = &port_state->current_response;

    if (port_state->status == Status_Idle || port_state->status == Status_Done) {
        // Normally we'd expect to be in receiving state and fail otherwise,
        // but the timing on that is a bit, well, asynchronous.
        // Being a bit lenient here and just checking that something is at least
        // meant to be happening here.

        // FIXME/TODO: ensure correct "receiving" status when expected        

        Error_Handler(252);
        return; // Although we'd never hit this point.
    }

    port_state->status = Status_Done;

    resp->comport_id = port_state->comport_id;
    resp->data = req->response_data;
    resp->data_length = port_state->current_request.response_len;
    resp->request_command = req->request_command;

    queue_add(resp);

    SWITCH_PORTS_IF_DONE();
}


// Switches between selected port pairs, and starts any queued requests
// for the ports previously unselected
static void switch_ports() {
    PortPair move_ports = unselected_ports;
    unselected_ports = selected_ports;
    selected_ports = move_ports;

    unselected_ports.first->selected = false;
    unselected_ports.second->selected = false;

    uart_connect_port(selected_ports.first->comport_id);
    uart_connect_port(selected_ports.second->comport_id);

    selected_ports.first->selected = true;
    selected_ports.second->selected = true;

    unselected_ports.first->status = Status_Idle;
    unselected_ports.second->status = Status_Idle;

    uint8_t any_reqs = false;
    // If newly-selected ports had queued requests, start them off now
    if (selected_ports.first->req_queue.count > 0) {
        Request req = req_queue_take(&selected_ports.first->req_queue);
        selected_ports.first->current_request = req;
        any_reqs = true;
        start_request(&selected_ports.first->current_request);
    } 

    if (selected_ports.second->req_queue.count > 0) {
        Request req = req_queue_take(&selected_ports.second->req_queue);
        selected_ports.second->current_request = req;
        any_reqs = true;
        start_request(&selected_ports.second->current_request);
    }
}

// Begin a new request
static void start_request(Request * request) {
    // Assumption at this stage: request has valid comport_id

    PortState * port_state = get_port_state(request->comport_id);

    port_state->status = Status_Sending_Command;

    if (request->send_data_len == 0 && request->response_len > 0) {
        uart_receive(
            request->comport_id,
            request->response_data,
            request->response_len
        );
    }

    uart_send(
        request->comport_id,
        &request->request_command, // Address of command byte in memory
        1 // number of bytes to send
    );

}

static PortState * get_port_state(ComportId comport_id) {
    switch (comport_id) {
        case Comport_Left: return &port_state_left;
        case Comport_Down: return &port_state_down;
        case Comport_Up: return &port_state_up;
        case Comport_Right: return &port_state_right;
        default: return NULL;
    }
}

static void queue_add(Response * resp) {
    // Don't take more responses than max
    if (queue_count == RESPONSE_QUEUE_MAX) return;

    if (queue_rear == RESPONSE_QUEUE_MAX - 1) {
        queue_rear = -1;
    }

    queue_rear++;
    queue_responses[queue_rear] = resp;
    queue_count++;
}

static Response * queue_take() {
    if (queue_count == 0) return NULL;

    Response * resp = queue_responses[queue_front];
    queue_front++;

    if (queue_front == RESPONSE_QUEUE_MAX) {
        queue_front = 0;
    }

    queue_count--;
    return resp;
} 
