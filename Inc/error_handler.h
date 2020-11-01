#ifndef __ERROR_HANDLER_H
#define __ERROR_HANDLER_H

#include "debug_leds.h"
#include "stm32f3xx.h"

typedef enum {
    Error_None                             = 0x0000,

    Error_Cortex_HardFault                 = 0x1001,
    Error_Cortex_MemManage                 = 0x1002,
    Error_Cortex_BusFault                  = 0x1003,
    Error_Cortex_UsageFault                = 0x1004,

    Error_HAL_RCC_OscConfig                = 0x1101,
    Error_HAL_RCC_ClockConfig              = 0x1102,
    Error_HAL_RCC_PeriphClockConfig        = 0x1103,
    Error_HAL_DMA_Init                     = 0x1104,
    Error_HAL_UART_Transmit_DMA            = 0x1105,
    Error_HAL_UART_Receive_DMA             = 0x1106,
    Error_HAL_UART_Init                    = 0x1107, 
    Error_HAL_UART_DeInit                  = 0x1108,
    Error_HAL_PCD_Init                     = 0x1109,

    Error_USB_USBD_Init                    = 0x1201,
    Error_USB_USBD_RegisterClass           = 0x1202,
    Error_USB_USBD_RegisterInterface       = 0x1203,
    Error_USB_USBD_Start                   = 0x1204,
    Error_USB_USBD_ConfWrongSpeed          = 0x1205,

    Error_App_UART_InvalidComport          = 0x2101,

    Error_App_MsgBus_InvalidComport        = 0x2201,
    Error_App_MsgBus_SendCpltInvalidStatus = 0x2202,
    Error_App_MsgBus_RecvCpltInvalidStatus = 0x2203,
    Error_App_MsgBus_RecvCpltNoAck         = 0x2204,

    Error_App_ReqQueue_QueueFull           = 0x2205
} ErrorCode;

volatile ErrorCode Panic_Error;
volatile uint32_t Panic_Data;

static inline void error_loop() {
    while (1) {
        // TODO: support doing error-specific blink patterns
        DBG_LED3_TOGGLE();
        HAL_Delay(250);
    }
}

static inline void error_panic_data(ErrorCode code, uint32_t data) {
    Panic_Error = code;
    Panic_Data = data;
    error_loop();
}

static inline void error_panic(ErrorCode code) {
    Panic_Error = code;
    error_loop();
}


#endif