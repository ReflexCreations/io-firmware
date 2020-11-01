/**
  ******************************************************************************
  * @file           : usbd_conf.c
  * @version        : v2.0_Cube
  * @brief          : This file implements the board support package for the USB
  *                   device library
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

#include "stm32f3xx.h"
#include "stm32f3xx_hal.h"
#include "usbd_def.h"
#include "usbd_core.h"
#include "usbd_customhid.h"
#include "error_handler.h"

PCD_HandleTypeDef hpcd_USB_FS;

static USBD_StatusTypeDef USBD_Get_USB_Status(HAL_StatusTypeDef hal_status);

#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCDEx_SetConnectionState(PCD_HandleTypeDef *hpcd, uint8_t state);
#else
void HAL_PCDEx_SetConnectionState(PCD_HandleTypeDef *hpcd, uint8_t state);
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */

/*******************************************************************************
                       LL Driver Callbacks (PCD -> USB Device Library)
*******************************************************************************/

void HAL_PCD_MspInit(PCD_HandleTypeDef* pcdHandle) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (pcdHandle->Instance == USB) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        // USB GPIO Configuration    
        // PA11 -> USB_DM
        // PA12 -> USB_DP 
        GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF14_USB;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        __HAL_RCC_USB_CLK_ENABLE();

        HAL_NVIC_SetPriority(USB_LP_CAN_RX0_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USB_LP_CAN_RX0_IRQn);
    }
}

void HAL_PCD_MspDeInit(PCD_HandleTypeDef* pcdHandle) {
  if (pcdHandle->Instance == USB) {
      __HAL_RCC_USB_CLK_DISABLE();
  
        // USB GPIO Configuration    
        // PA11     ------> USB_DM
        // PA12     ------> USB_DP 
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11 | GPIO_PIN_12);
        HAL_NVIC_DisableIRQ(USB_LP_CAN_RX0_IRQn);
    }
}

// Setup stage callback
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd) {
#else
void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd) {
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
    USBD_LL_SetupStage(
        (USBD_HandleTypeDef*)hpcd->pData, (uint8_t *)hpcd->Setup
    );
}

// Data Out stage callback
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
#else
void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
    USBD_LL_DataOutStage(
        (USBD_HandleTypeDef*)hpcd->pData,
        epnum,
        hpcd->OUT_ep[epnum].xfer_buff
    );
}

// Data in stage callback
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
#else
void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
    USBD_LL_DataInStage(
        (USBD_HandleTypeDef*)hpcd->pData,
        epnum,
        hpcd->IN_ep[epnum].xfer_buff
    );
}

// SOF callback
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_SOFCallback(PCD_HandleTypeDef *hpcd) {
#else
void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd) {
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
    USBD_LL_SOF((USBD_HandleTypeDef*)hpcd->pData);
}

// Reset callback
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_ResetCallback(PCD_HandleTypeDef *hpcd) {
#else
void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd) {
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
    USBD_SpeedTypeDef speed = USBD_SPEED_FULL;

    if (hpcd->Init.speed != PCD_SPEED_FULL) {
        error_panic_data(Error_USB_USBD_ConfWrongSpeed, hpcd->Init.speed);
    }

    // Set speed
    USBD_LL_SetSpeed((USBD_HandleTypeDef*)hpcd->pData, speed);

    // Reset
    USBD_LL_Reset((USBD_HandleTypeDef*)hpcd->pData);
}

// Suspend callback
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_SuspendCallback(PCD_HandleTypeDef *hpcd) {
#else
void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd) {
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
    // Inform USB library that core enters in suspend Mode.
    USBD_LL_Suspend((USBD_HandleTypeDef*)hpcd->pData);
    // Enter in STOP mode.
    if (hpcd->Init.low_power_enable) {
        // Set SLEEPDEEP bit and SleepOnExit of Cortex System Control Register.
        SCB->SCR |= (uint32_t)(
            (uint32_t)(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk)
        );
    }
}

// Resume callback
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_ResumeCallback(PCD_HandleTypeDef *hpcd) {
#else
void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd) {
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
    USBD_LL_Resume((USBD_HandleTypeDef*)hpcd->pData);
}

// ISOOUTIncomplete callback.
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_ISOOUTIncompleteCallback(
    PCD_HandleTypeDef *hpcd,
    uint8_t epnum
) {
#else
void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
    USBD_LL_IsoOUTIncomplete((USBD_HandleTypeDef*)hpcd->pData, epnum);
}

// ISOINIncomplete callback.
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_ISOINIncompleteCallback(
    PCD_HandleTypeDef *hpcd,
    uint8_t epnum
) {
#else
void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
    USBD_LL_IsoINIncomplete((USBD_HandleTypeDef*)hpcd->pData, epnum);
}

// Connect callback.
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_ConnectCallback(PCD_HandleTypeDef *hpcd) {
#else
void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd) {
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
    USBD_LL_DevConnected((USBD_HandleTypeDef*)hpcd->pData);
}

// Disconnect callback.
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
static void PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd) {
#else
void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd) {
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
    USBD_LL_DevDisconnected((USBD_HandleTypeDef*)hpcd->pData);
}

/*******************************************************************************
                       LL Driver Interface (USB Device Library --> PCD)
*******************************************************************************/

// Initializes the low level portion of the device driver.
USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *pdev) {
    /* Init USB Ip. */
    /* Link the driver to the stack. */
    hpcd_USB_FS.pData = pdev;
    pdev->pData = &hpcd_USB_FS;

    hpcd_USB_FS.Instance = USB;
    hpcd_USB_FS.Init.dev_endpoints = 8;
    hpcd_USB_FS.Init.speed = PCD_SPEED_FULL;
    hpcd_USB_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
    hpcd_USB_FS.Init.low_power_enable = DISABLE;
    hpcd_USB_FS.Init.battery_charging_enable = DISABLE;

    HAL_StatusTypeDef res = HAL_PCD_Init(&hpcd_USB_FS);
    if (res != HAL_OK) {
        error_panic_data(Error_HAL_PCD_Init, (uint32_t)res);
    }

#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
    /* Register USB PCD CallBacks */
    HAL_PCD_RegisterCallback(
        &hpcd_USB_FS, HAL_PCD_SOF_CB_ID, PCD_SOFCallback);
    HAL_PCD_RegisterCallback(
        &hpcd_USB_FS, HAL_PCD_SETUPSTAGE_CB_ID, PCD_SetupStageCallback);
    HAL_PCD_RegisterCallback(
        &hpcd_USB_FS, HAL_PCD_RESET_CB_ID, PCD_ResetCallback);
    HAL_PCD_RegisterCallback(
        &hpcd_USB_FS, HAL_PCD_SUSPEND_CB_ID, PCD_SuspendCallback);
    HAL_PCD_RegisterCallback(
        &hpcd_USB_FS, HAL_PCD_RESUME_CB_ID, PCD_ResumeCallback);
    HAL_PCD_RegisterCallback(
        &hpcd_USB_FS, HAL_PCD_CONNECT_CB_ID, PCD_ConnectCallback);
    HAL_PCD_RegisterCallback(
        &hpcd_USB_FS, HAL_PCD_DISCONNECT_CB_ID, PCD_DisconnectCallback);

    HAL_PCD_RegisterDataOutStageCallback(
        &hpcd_USB_FS, PCD_DataOutStageCallback);
    HAL_PCD_RegisterDataInStageCallback(
        &hpcd_USB_FS, PCD_DataInStageCallback);
    HAL_PCD_RegisterIsoOutIncpltCallback(
        &hpcd_USB_FS, PCD_ISOOUTIncompleteCallback);
    HAL_PCD_RegisterIsoInIncpltCallback(
        &hpcd_USB_FS, PCD_ISOINIncompleteCallback);
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
    HAL_PCDEx_PMAConfig(
        (PCD_HandleTypeDef*)pdev->pData, 0x00, PCD_SNG_BUF, 0x18);
    HAL_PCDEx_PMAConfig(
        (PCD_HandleTypeDef*)pdev->pData, 0x80, PCD_SNG_BUF, 0x58);
    HAL_PCDEx_PMAConfig(
        (PCD_HandleTypeDef*)pdev->pData,
        CUSTOM_HID_EPIN_ADDR,
        PCD_SNG_BUF,
        0x98
    );
    HAL_PCDEx_PMAConfig(
        (PCD_HandleTypeDef*)pdev->pData,
        CUSTOM_HID_EPOUT_ADDR,
        PCD_SNG_BUF,
        0xD8
    );

    return USBD_OK;
}

// De-Initializes the low level portion of the device driver.
USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *pdev) {
    HAL_StatusTypeDef hal_status = HAL_PCD_DeInit(pdev->pData);
    return USBD_Get_USB_Status(hal_status);
}

// Starts the low level portion of the device driver.
USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *pdev) {
    HAL_StatusTypeDef hal_status = HAL_PCD_Start(pdev->pData);
    return USBD_Get_USB_Status(hal_status);  
}

// Stops the low level portion of the device driver.
USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *pdev) {
    HAL_StatusTypeDef hal_status = HAL_PCD_Stop(pdev->pData);
    return USBD_Get_USB_Status(hal_status);
}

// Opens an endpoint of the low level driver.
USBD_StatusTypeDef USBD_LL_OpenEP(
    USBD_HandleTypeDef *pdev,
    uint8_t ep_addr,
    uint8_t ep_type,
    uint16_t ep_mps
) {
    HAL_StatusTypeDef hal_status = HAL_PCD_EP_Open(
        pdev->pData, ep_addr, ep_mps, ep_type);

    return USBD_Get_USB_Status(hal_status);
}

// Closes an endpoint of the low level driver.
USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
    HAL_StatusTypeDef hal_status = HAL_PCD_EP_Close(pdev->pData, ep_addr);
    return USBD_Get_USB_Status(hal_status);
}

// Flushes an endpoint of the Low Level Driver.
USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
    HAL_StatusTypeDef hal_status = HAL_PCD_EP_Flush(pdev->pData, ep_addr);
    return USBD_Get_USB_Status(hal_status);
}

// Sets a Stall condition on an endpoint of the Low Level Driver.
USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
    HAL_StatusTypeDef hal_status = HAL_PCD_EP_SetStall(pdev->pData, ep_addr);
    return USBD_Get_USB_Status(hal_status);
}

// Clears a Stall condition on an endpoint of the Low Level Driver.
USBD_StatusTypeDef USBD_LL_ClearStallEP(
    USBD_HandleTypeDef *pdev,
    uint8_t ep_addr
) {
    HAL_StatusTypeDef hal_status = HAL_PCD_EP_ClrStall(pdev->pData, ep_addr);  
    return USBD_Get_USB_Status(hal_status);
}

// Returns Stall condition.
uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
    PCD_HandleTypeDef *hpcd = (PCD_HandleTypeDef*) pdev->pData;
    
    if ((ep_addr & 0x80) == 0x80) {
        return hpcd->IN_ep[ep_addr & 0x7F].is_stall; 
    } 

    return hpcd->OUT_ep[ep_addr & 0x7F].is_stall; 
}

// Assigns a USB address to the device.
USBD_StatusTypeDef USBD_LL_SetUSBAddress(
  USBD_HandleTypeDef *pdev,
  uint8_t dev_addr
) {
    HAL_StatusTypeDef hal_status = HAL_PCD_SetAddress(pdev->pData, dev_addr);
    return USBD_Get_USB_Status(hal_status);
}

// Transmits data over an endpoint.
USBD_StatusTypeDef USBD_LL_Transmit(
    USBD_HandleTypeDef *pdev,
    uint8_t ep_addr,
    uint8_t *pbuf,
    uint16_t size
) {
    HAL_StatusTypeDef hal_status = HAL_PCD_EP_Transmit(
        pdev->pData,
        ep_addr,
        pbuf,
        size
    );

    return USBD_Get_USB_Status(hal_status);
}

// Prepares an endpoint for reception.
USBD_StatusTypeDef USBD_LL_PrepareReceive(
    USBD_HandleTypeDef *pdev,
    uint8_t ep_addr,
    uint8_t *pbuf,
    uint16_t size
) {
    HAL_StatusTypeDef hal_status = HAL_PCD_EP_Receive(
        pdev->pData,
        ep_addr,
        pbuf,
        size
    );

    return USBD_Get_USB_Status(hal_status);
}

// Returns the last transfered packet size.
uint32_t USBD_LL_GetRxDataSize(USBD_HandleTypeDef *pdev, uint8_t ep_addr) {
    return HAL_PCD_EP_GetRxCount((PCD_HandleTypeDef*) pdev->pData, ep_addr);
}

// Delays routine for the USB device library.
void USBD_LL_Delay(uint32_t Delay) {
  HAL_Delay(Delay);
}

// Static single allocation.
void *USBD_static_malloc(uint32_t size) {
    // On 32-bit boundary
    static uint32_t mem[(sizeof(USBD_CUSTOM_HID_HandleTypeDef) / 4 + 1)];
    return mem;
}

// Dummy memory free
void USBD_static_free(void *p) { }

// Software device connection
#if (USE_HAL_PCD_REGISTER_CALLBACKS == 1U)
void PCDEx_SetConnectionState(PCD_HandleTypeDef *hpcd, uint8_t state) {
#else
void HAL_PCDEx_SetConnectionState(PCD_HandleTypeDef *hpcd, uint8_t state) {
#endif /* USE_HAL_PCD_REGISTER_CALLBACKS */
    if (state == 1) {
    } else {
    }
}

// Retuns the USB status depending on the HAL status:
USBD_StatusTypeDef USBD_Get_USB_Status(HAL_StatusTypeDef hal_status) {
    switch (hal_status)  {
        case HAL_OK: return USBD_OK;
        case HAL_BUSY: return USBD_BUSY;

        case HAL_ERROR: // Fallthrough intentional
        case HAL_TIMEOUT: // Fallthrough intentional
        default: return USBD_FAIL;
    }
}