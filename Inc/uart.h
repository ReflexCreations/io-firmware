#ifndef __UART__H
#define __UART__H

#include "stm32f3xx.h"
#include "bool.h"

// The numeric value assigned are important to
// msgbus; don't change them.
// They are used for offsetting in memory.
typedef enum {
    Comport_None  = 0xFF,
    Comport_Left  = 0x00,
    Comport_Down  = 0x01,
    Comport_Up    = 0x02,
    Comport_Right = 0x03
} ComportId;

typedef void (* SendCompleteHandler)(ComportId);
typedef void (* ReceiveCompleteHandler)(ComportId);

// Initializes uart functionality
void uart_init();

// Ensures a given port is electrically connected to a UART peripheral
void uart_connect_port(ComportId);

// Begins sending data to a given comport
// data_ptr points to the start of an array of data to be sent,
// data_len is the number of bytes to be sent
void uart_send(
    ComportId comport_id, uint8_t * data_ptr, uint16_t data_len);

// Begins receiving on a given comport
// data_ptr points to the start of an array where received data will be stored
// data_len is the number of bytes we expect to receive
void uart_receive(
    ComportId comport_id, uint8_t * data_ptr, uint16_t data_len);

// Configures a callback function to be called when a transmission completes
// The handler function will be passed the ComportId of the port that finished
// transmitting
void uart_set_on_send_complete_handler(SendCompleteHandler);

// Configures a callback function to be called when a receive completes
// The handler function will be be passed the ComportId of the port that
// finished receiving
void uart_set_on_receive_complete_handler(ReceiveCompleteHandler);

#endif