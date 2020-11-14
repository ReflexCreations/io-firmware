/**
  ******************************************************************************
  * @file    stm32f3xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

#include "uart.h"
#include "stm32f3xx_it.h"
#include "error_handler.h"

// uart.c
extern DMA_HandleTypeDef hdma_usart1_l_rx;
extern DMA_HandleTypeDef hdma_usart1_l_tx;
extern DMA_HandleTypeDef hdma_usart2_u_r_rx;
extern DMA_HandleTypeDef hdma_usart2_u_r_tx;
extern DMA_HandleTypeDef hdma_usart3_d_rx;
extern DMA_HandleTypeDef hdma_usart3_d_tx;

extern UART_HandleTypeDef huart1_l;
extern UART_HandleTypeDef huart2_u_r;
extern UART_HandleTypeDef huart3_d;

// All names of interrupts are defined in the startup file.

// Cortex-M4 Core interrupt / exception handlers -------------------------------

// Non-maskable interrupt handler
void NMI_Handler(void) { }

void HardFault_Handler(void) { 
    error_panic(Error_Cortex_HardFault);
}

// Memory Management fault handler
void MemManage_Handler(void) {
    error_panic(Error_Cortex_MemManage);
}

// Pre-fetch fault, memory access fault handler
void BusFault_Handler(void) {
    error_panic(Error_Cortex_BusFault);
}

// Undefined instruction, illegal state handler
void UsageFault_Handler(void) {
    error_panic(Error_Cortex_UsageFault);
 }

// System service call via SWI instruction handler
void SVC_Handler(void) { }

// Debug monitor handler
void DebugMon_Handler(void) {  }

// Pendable request for system service handler
void PendSV_Handler(void) { }

void SysTick_Handler(void) { HAL_IncTick(); }

// STM32F3xx Peripheral Interrupt Handlers -------------------------------------

// USART3_TX (Down)
void DMA1_Channel2_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_usart3_d_tx);
}

// USART3_RX (Down)
void DMA1_Channel3_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_usart3_d_rx);
}

// USART1_TX (Left)
void DMA1_Channel4_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_usart1_l_tx);
}

// USART1_RX (Left)
void DMA1_Channel5_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_usart1_l_rx);
}

// USART2_RX (Up, Right)
void DMA1_Channel6_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_usart2_u_r_rx);
}

// USART2_TX (Up, Right)
void DMA1_Channel7_IRQHandler(void) {
    HAL_DMA_IRQHandler(&hdma_usart2_u_r_tx);
}

// Left
void USART1_IRQHandler() { 
    HAL_UART_IRQHandler(&huart1_l);
}

// Up, Right
void USART2_IRQHandler() {
    HAL_UART_IRQHandler(&huart2_u_r);
}

// Down
void USART3_IRQHandler() {
    HAL_UART_IRQHandler(&huart3_d);
}