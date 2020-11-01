/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : stm32f3xx_hal_msp.c
  * Description        : This file provides code for the MSP Initialization 
  *                      and de-Initialization codes.
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
#include "error_handler.h"

extern DMA_HandleTypeDef hdma_usart1_l_rx;
extern DMA_HandleTypeDef hdma_usart1_l_tx;
extern DMA_HandleTypeDef hdma_usart2_u_r_rx;
extern DMA_HandleTypeDef hdma_usart2_u_r_tx;
extern DMA_HandleTypeDef hdma_usart3_d_rx;
extern DMA_HandleTypeDef hdma_usart3_d_tx;

// Initializes the Global MSP.
void HAL_MspInit() {
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
}

// UART MSP Initialization
// Configures the hardware resources for the UART peripherals used
void HAL_UART_MspInit(UART_HandleTypeDef* huart) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        // USART1 GPIO Configuration    
        // PA9  -> USART1_TX
        // PA10 -> USART1_RX 
        GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        // USART1 DMA Init

        // USART1_RX Init
        hdma_usart1_l_rx.Instance = DMA1_Channel5;
        hdma_usart1_l_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart1_l_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart1_l_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart1_l_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart1_l_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart1_l_rx.Init.Mode = DMA_NORMAL;
        hdma_usart1_l_rx.Init.Priority = DMA_PRIORITY_LOW;

        if (HAL_DMA_Init(&hdma_usart1_l_rx) != HAL_OK) {
            error_panic_data(Error_HAL_DMA_Init, (uint32_t)DMA1_Channel5);
        }

        __HAL_LINKDMA(huart,hdmarx,hdma_usart1_l_rx);

        /* USART1_TX Init */
        hdma_usart1_l_tx.Instance = DMA1_Channel4;
        hdma_usart1_l_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart1_l_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart1_l_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart1_l_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart1_l_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart1_l_tx.Init.Mode = DMA_NORMAL;
        hdma_usart1_l_tx.Init.Priority = DMA_PRIORITY_LOW;

        if (HAL_DMA_Init(&hdma_usart1_l_tx) != HAL_OK) {
            error_panic_data(Error_HAL_DMA_Init, (uint32_t)DMA1_Channel4);
        }

        __HAL_LINKDMA(huart,hdmatx,hdma_usart1_l_tx);

    } else if (huart->Instance == USART2) {
        __HAL_RCC_USART2_CLK_ENABLE();

        // TODO: GPIO config should be here, too
      
        // USART2 DMA Init

        // USART2_RX Init 
        hdma_usart2_u_r_rx.Instance = DMA1_Channel6;
        hdma_usart2_u_r_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart2_u_r_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart2_u_r_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart2_u_r_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart2_u_r_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart2_u_r_rx.Init.Mode = DMA_NORMAL;
        hdma_usart2_u_r_rx.Init.Priority = DMA_PRIORITY_LOW;

        if (HAL_DMA_Init(&hdma_usart2_u_r_rx) != HAL_OK) {
            error_panic_data(Error_HAL_DMA_Init, (uint32_t)DMA1_Channel6);
        }

        __HAL_LINKDMA(huart, hdmarx, hdma_usart2_u_r_rx);

        // USART2_TX Init
        hdma_usart2_u_r_tx.Instance = DMA1_Channel7;
        hdma_usart2_u_r_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart2_u_r_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart2_u_r_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart2_u_r_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart2_u_r_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart2_u_r_tx.Init.Mode = DMA_NORMAL;
        hdma_usart2_u_r_tx.Init.Priority = DMA_PRIORITY_LOW;

        if (HAL_DMA_Init(&hdma_usart2_u_r_tx) != HAL_OK) {
            error_panic_data(Error_HAL_DMA_Init, (uint32_t)DMA1_Channel7);
        }

        __HAL_LINKDMA(huart, hdmatx, hdma_usart2_u_r_tx);

    } else if (huart->Instance == USART3) {
        __HAL_RCC_USART3_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        // USART3 GPIO Configuration    
        // PB10 -> USART3_TX
        // PB11 -> USART3_RX 
        GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        // USART3 DMA Init

        // USART3_RX Init
        hdma_usart3_d_rx.Instance = DMA1_Channel3;
        hdma_usart3_d_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart3_d_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart3_d_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart3_d_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart3_d_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart3_d_rx.Init.Mode = DMA_NORMAL;
        hdma_usart3_d_rx.Init.Priority = DMA_PRIORITY_LOW;

        if (HAL_DMA_Init(&hdma_usart3_d_rx) != HAL_OK) {
            error_panic_data(Error_HAL_DMA_Init, (uint32_t)DMA1_Channel3);
        }

        __HAL_LINKDMA(huart,hdmarx,hdma_usart3_d_rx);

        // USART3_TX Init
        hdma_usart3_d_tx.Instance = DMA1_Channel2;
        hdma_usart3_d_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart3_d_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart3_d_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart3_d_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart3_d_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart3_d_tx.Init.Mode = DMA_NORMAL;
        hdma_usart3_d_tx.Init.Priority = DMA_PRIORITY_LOW;

        if (HAL_DMA_Init(&hdma_usart3_d_tx) != HAL_OK) {
            error_panic_data(Error_HAL_DMA_Init, (uint32_t)DMA1_Channel2);
        }

        __HAL_LINKDMA(huart, hdmatx, hdma_usart3_d_tx);
    }
}

// UART MSP De-Initialization
// Freezes the hardware resources for the UART peripherals used
void HAL_UART_MspDeInit(UART_HandleTypeDef* huart) {
    if (huart->Instance == USART1) {

        __HAL_RCC_USART1_CLK_DISABLE();
      
        // USART1 GPIO Configuration    
        // PA9  -> USART1_TX
        // PA10 -> USART1_RX 
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);

        // USART1 DMA DeInit
        HAL_DMA_DeInit(huart->hdmarx);
        HAL_DMA_DeInit(huart->hdmatx);

    } else if (huart->Instance == USART2) {

        __HAL_RCC_USART2_CLK_DISABLE();
      
        // USART2 GPIO Configuration    
        // PA2 -> USART2_TX
        // PA3 -> USART2_RX 
        // TODO: actual GPIO de-init here? 

        // USART2 DMA DeInit
        HAL_DMA_DeInit(huart->hdmarx);
        HAL_DMA_DeInit(huart->hdmatx);

    } else if (huart->Instance == USART3) {

        __HAL_RCC_USART3_CLK_DISABLE();
      
        // USART3 GPIO Configuration    
        // PB10 -> USART3_TX
        // PB11 -> USART3_RX 
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10 | GPIO_PIN_11);

        // USART3 DMA DeInit
        HAL_DMA_DeInit(huart->hdmarx);
        HAL_DMA_DeInit(huart->hdmatx);
    }
}