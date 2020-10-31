#ifndef __CONFIG_H
#define __CONFIG_H

#include "stm32f3xx.h"
#include "uart.h"

// File for configuration #define FLAGS


// Change these if depending on what you've got plugged in
#define PANEL_LEFT_CONNECTED  (0U) 
#define PANEL_UP_CONNECTED    (0U) 
#define PANEL_DOWN_CONNECTED  (1U)
#define PANEL_RIGHT_CONNECTED (1U)

extern uint8_t _panels_connected[4];

inline uint8_t panel_connected(ComportId port) {
    if (port == Comport_None) return 0U;
    return _panels_connected[(uint8_t)port];
}

#endif