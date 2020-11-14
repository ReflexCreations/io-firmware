#include "msgbus.h"
#include "uart.h"
#include "request.h"
#include "req_queue.h"
#include "error_handler.h"
#include "config.h"

#define RESPONSE_QUEUE_MAX (4U)
#define RESPONSE_TIMEOUT_TICKS (2U)

#define SEND_COMPLETE_MASK (0x01U)
#define RECEIVE_COMPLETE_MASK (0x02U)

// Public, so that contents can be inspected during debugging
PortState port_state_left;
PortState port_state_down;
PortState port_state_up;
PortState port_state_right;

static PortState * port_states[COMPORT_ID_MAX + 1];

static PortPair selected_ports;
static PortPair unselected_ports;

static Response * queue_responses[RESPONSE_QUEUE_MAX];
static int8_t queue_front = 0;
static int8_t queue_rear = -1;
static uint8_t queue_count = 0;

static void switch_ports();
static void start_request(Request *);
static PortState * get_port_state(ComportId);

static void check_timeout(PortState *);

// Should be given to uart as function pointers
static void uart_on_send_complete(ComportId);
static void uart_on_receive_complete(ComportId);

static void process_send_complete(PortState *);
static void process_receive_complete(PortState *);

static void queue_add(Response *);
static Response * queue_take();


static inline Response create_response(
    ComportId port,
    Commands request_command,
    uint8_t * data,
    uint16_t data_length
) {
    Response resp;
    resp.comport_id = port;
    resp.request_command = request_command;
    resp.data = data;
    resp.data_length = data_length;

    return resp;
}

static inline Response create_blank_response(ComportId port) {
    return create_response(port, Command_None, NULL, 0);
}

static inline void init_port_state(
    PortState * state,
    ComportId port,
    uint8_t selected
) {
    state->comport_id = port;
    state->status = Status_Idle;
    state->selected = selected;
    state->current_request = request_create(Command_None);
    state->current_request.comport_id = port;
    state->current_response = create_blank_response(port);
    state->interrupt_flags = 0x00;
    req_queue_init(&state->req_queue);
}

static inline PortState * get_port_state(ComportId comport_id) {
    if (comport_id > COMPORT_ID_MAX) {
        error_panic_data(Error_App_MsgBus_InvalidComport, comport_id);
    }

    return port_states[comport_id];
}

static inline void switch_ports_if_done() {
    PortStatus status1 = selected_ports.first->status;
    PortStatus status2 = selected_ports.second->status;

    if ((status1 == Status_Idle || status1 == Status_Done) &&
        (status2 == Status_Idle || status2 == Status_Done)) {

        switch_ports();
    }
}

static inline void expect_acknowledge(PortState * port_state) {
    uart_receive(port_state->comport_id, port_state->acknowledged, 1);
}

static inline void expect_acknowledge_command(PortState * port_state) {
    uart_receive(port_state->comport_id, port_state->acknowledged, 2);
}

static inline uint8_t check_acknowledge(PortState * port_state) {
    uint8_t ack_was = port_state->acknowledged[0];
    Commands ack_cmd_was = port_state->acknowledged[1];
    port_state->acknowledged[0] = 0x00;
    return ack_was == MSG_ACKNOWLEGE 
            && ack_cmd_was == port_state->current_request.request_command;
}

static inline void clear_acknowledge_command(PortState * port_state) {
    port_state->acknowledged[1] = Command_None;
}

// Helpers for dealing with flags set by interrupts
static inline uint8_t send_complete_is_set(PortState * port_state) {
    return port_state->interrupt_flags & SEND_COMPLETE_MASK;
}

static inline uint8_t receive_complete_is_set(PortState * port_state) {
    return port_state->interrupt_flags & RECEIVE_COMPLETE_MASK;
}

static inline void set_send_complete(PortState * port_state) {
    port_state->interrupt_flags |= SEND_COMPLETE_MASK;
}

static inline void reset_send_complete(PortState * port_state) {
    port_state->interrupt_flags &= ~SEND_COMPLETE_MASK;
}

static inline void set_receive_complete(PortState * port_state) {
    port_state->interrupt_flags |= RECEIVE_COMPLETE_MASK;
}

static inline void reset_receive_complete(PortState * port_state) {
    port_state->interrupt_flags &= ~RECEIVE_COMPLETE_MASK;
}

// Whether any of the ports have interrupt flags
static inline uint8_t any_interrupt_flags() {
    return port_state_left.interrupt_flags 
            || port_state_down.interrupt_flags
            || port_state_up.interrupt_flags
            || port_state_right.interrupt_flags;
}

// Processes interrupt flags that were set since the last call,
// set by a send and/or receive transaction completing
static inline void process_flags(PortState * port_state) {
    // Note: it's important that process_send_complete is called before
    // process_receive_complete, for correct function of the state machine.
    if (send_complete_is_set(port_state)) {
        reset_send_complete(port_state);
        process_send_complete(port_state);
    }

    if (receive_complete_is_set(port_state)) {
        reset_receive_complete(port_state);
        process_receive_complete(port_state);
    }
}

// Public functions ------------------------------------------------------------

void msgbus_init() {
    init_port_state(&port_state_left, Comport_Left, true);
    init_port_state(&port_state_up, Comport_Up, true);
    init_port_state(&port_state_right, Comport_Right, false);
    init_port_state(&port_state_down, Comport_Down, false);

    port_states[Comport_Left] = &port_state_left;
    port_states[Comport_Down] = &port_state_down;
    port_states[Comport_Up] = &port_state_up;
    port_states[Comport_Right] = &port_state_right;

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
    if (!any_interrupt_flags()) {
        check_timeout(&port_state_left);
        check_timeout(&port_state_down);
        check_timeout(&port_state_up);
        check_timeout(&port_state_right);
        switch_ports_if_done();
        return;
    }

    process_flags(&port_state_left);
    process_flags(&port_state_down);
    process_flags(&port_state_up);
    process_flags(&port_state_right);

    switch_ports_if_done();
}

void msgbus_send_request(Request request) {
    if (!panel_connected(request.comport_id)) return;

    PortState * portState = get_port_state(request.comport_id);

    // Not Idle? Stick it on the queue
    // Also if port is not selected we'll queue it for later
    if (portState->status != Status_Idle || !portState->selected) {
        // Only queue a request if it's not one that's currently being
        // executed
        if (!request_equals(portState->current_request, request)) {
            req_queue_add(&portState->req_queue, request);
        }

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
    switch_ports_if_done();
}

PortStatus msgbus_port_status(ComportId comport_id) {
    return get_port_state(comport_id)->status;
}

void msgbus_wait_for_idle(ComportId comport_id) {
    PortState * port_state = get_port_state(comport_id);

    while (port_state->status != Status_Idle
            || port_state->req_queue.count > 0) {
        msgbus_process_flags();
    }
}

// Private functions -----------------------------------------------------------

// Callbacks for uart interrupts
static void uart_on_send_complete(ComportId comport_id) {
    set_send_complete(get_port_state(comport_id));
}

static void uart_on_receive_complete(ComportId comport_id) {
    set_receive_complete(get_port_state(comport_id));
}

// Process interrupt flags on main thread

// Process a completed UART send
static void process_send_complete(PortState * port_state) {
    Request * req = &port_state->current_request;

    switch (port_state->status) {

        case Status_Sending_Command:
            if (!request_has_data(req) && request_expects_response(req)) {
                port_state->status = Status_Receiving;
            } else {
                port_state->status = Status_Awaiting_Command_Ack;
            }

            port_state->waiting_since = HAL_GetTick();

            break;

        case Status_Sending_Data:
            // Finished sending additional data, now see if we should expect
            // a response right now.
            if (request_expects_response(req)) {
                port_state->status = Status_Receiving;
            } else {
                port_state->status = Status_Awaiting_Data_Ack;
            }

            port_state->waiting_since = HAL_GetTick();

            break;

        default:
            // If we're getting this callback when not in either of the
            // sending statuses, something is wrong. Panic.
            error_panic_data(
                Error_App_MsgBus_SendCpltInvalidStatus,
                port_state->status
            );

            break;
    }
}

static void process_receive_complete(PortState * port_state) {
    Request * req = &port_state->current_request;

    switch (port_state->status) {
        case Status_Awaiting_Command_Ack:
            if (!check_acknowledge(port_state)) {
                error_panic_data(
                    Error_App_MsgBus_RecvCpltNoAck,
                    Status_Awaiting_Command_Ack
                );

                break;
            }

            // No more data to send? Then we're done.
            // We wouldn't be in awaiting ack state if we expected
            // a data response back without sending data out first.
            if (!request_has_data(req)) {
                port_state->status = Status_Done;
                break;
            }

            port_state->status = Status_Sending_Data;

            // If we also expect a response, set that up first now
            if (request_expects_response(req)) {
                uart_receive(
                    port_state->comport_id,
                    req->response_data,
                    req->response_len
                );
            } else {
                expect_acknowledge(port_state);
            }

            // Send our data payload
            uart_send(
                port_state->comport_id,
                req->send_data,
                req->send_data_len
            );
            
            break;

        case Status_Awaiting_Data_Ack:
            // If we get in this state at all, we're not expecting a data
            // response, so we can mark it done
            if (!check_acknowledge(port_state)) {
                error_panic_data(
                    Error_App_MsgBus_RecvCpltNoAck,
                    Status_Awaiting_Data_Ack
                );

                break;
            }

            port_state->status = Status_Done;
            break;

        case Status_Receiving:
            port_state->current_response = create_response(
                req->comport_id,
                req->request_command,
                req->response_data,
                req->response_len
            );

            queue_add(&port_state->current_response);
            port_state->status = Status_Done;
            break;

        default:
            // Not in one of the valid states, panic
            error_panic_data(
                Error_App_MsgBus_RecvCpltInvalidStatus,
                port_state->status
            );

            break;
    }
}

static void check_timeout(PortState * port_state) {
    switch (port_state->status) {
        case Status_Awaiting_Command_Ack:
        case Status_Awaiting_Data_Ack:
        case Status_Receiving:
            if (HAL_GetTick() - port_state->waiting_since \
                > RESPONSE_TIMEOUT_TICKS) {

                uart_abort_receive(port_state->comport_id);
                port_state->timeout_count++;
                port_state->status = Status_Done;
            }

            break;
    }
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

    // If newly-selected ports had queued requests, start them off now
    if (selected_ports.first->req_queue.count > 0) {
        Request req = req_queue_take(&selected_ports.first->req_queue);
        selected_ports.first->current_request = req;
        start_request(&selected_ports.first->current_request);
    } 

    if (selected_ports.second->req_queue.count > 0) {
        Request req = req_queue_take(&selected_ports.second->req_queue);
        selected_ports.second->current_request = req;
        start_request(&selected_ports.second->current_request);
    }
}

// Begin a new request
static void start_request(Request * request) {
    // Assumption at this stage: request has valid comport_id

    PortState * port_state = get_port_state(request->comport_id);
    clear_acknowledge_command(port_state);

    port_state->status = Status_Sending_Command;

    if (!request_has_data(request) && request_expects_response(request)) {
        uart_receive(
            request->comport_id,
            request->response_data,
            request->response_len
        );
    } else {
        // Expect the command to be acknowledged if we:
        //   either have more data to send, or don't have more data to send but
        //   also don't expect a data response
        // in essence, acknowledge is only redundant if we already expect the
        // panel to send something back right after getting the command.
        expect_acknowledge_command(port_state);
    }

    uart_send(request->comport_id, &request->request_command, 1);
}

static void queue_add(Response * resp) {
    // Don't take more responses than max
    if (queue_count == RESPONSE_QUEUE_MAX) return;

    if (queue_rear == RESPONSE_QUEUE_MAX - 1) queue_rear = -1;

    queue_rear++;
    queue_responses[queue_rear] = resp;
    queue_count++;
}

static Response * queue_take() {
    if (queue_count == 0) return NULL;

    Response * resp = queue_responses[queue_front];

    queue_front++;
    if (queue_front == RESPONSE_QUEUE_MAX) queue_front = 0;
    queue_count--;

    return resp;
} 