#ifndef __ERROR_HANDLER_H
#define __ERROR_HANDLER_H

#include "debug_leds.h"
#include "stm32f3xx.h"

volatile uint32_t error_handler_delay;

static inline void Error_Handler(uint16_t blink_delay) {
    error_handler_delay = blink_delay;

    while (1) {
        DBG_LED3_TOGGLE();
        HAL_Delay(error_handler_delay);
    }
}

#endif