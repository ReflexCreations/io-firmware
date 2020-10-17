#include "main.h"
#include "usb_device.h"
#include "usbd_customhid.h"
#include "bool.h"
#include "uart.h"
#include "msgbus.h"
#include "debug_leds.h"
#include "error_handler.h"

#define USB_HID_PACKET_SIZE_BYTES (64U)

#define BUILD_SENSOR_REQ(req, comport) \
  req.comport_id = comport; \
  req.request_command = CMD_REQUEST_SENSORS; \
  req.send_data = NULL; \
  req.send_data_len = 0; \
  req.response_data = sensor_buffer + ((uint8_t)comport * 8); \
  req.response_len = 8;

extern USBD_HandleTypeDef hUsbDeviceFS;

UART_HandleTypeDef huart1_l; // Left
UART_HandleTypeDef huart2_u_r; // Up, Right
UART_HandleTypeDef huart3_d; // Down
DMA_HandleTypeDef hdma_usart1_l_rx; // Left
DMA_HandleTypeDef hdma_usart1_l_tx;
DMA_HandleTypeDef hdma_usart2_u_r_rx; // Up, right
DMA_HandleTypeDef hdma_usart2_u_r_tx;
DMA_HandleTypeDef hdma_usart3_d_rx; // Down
DMA_HandleTypeDef hdma_usart3_d_tx;

uint8_t sensor_buffer[USB_HID_PACKET_SIZE_BYTES];
uint8_t usb_sensor_buffer[USB_HID_PACKET_SIZE_BYTES];

extern bool packet_received;

typedef enum {
  CMD_REQUEST_SENSORS = 0x10,
  CMD_TRANSMIT_LED_DATA = 0x20,
  CMD_COMMIT_LEDS = 0x30
} Commands;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);

int main(void){
  HAL_Init();
  MX_GPIO_Init();
  SystemClock_Config();
  uart_init();
  msgbus_init();
  
  MX_USB_DEVICE_Init();

  uint8_t led_info = 0x00;
  uint16_t segments_received = 0x0000;
  uint8_t last_frame = 0x00;

  USBD_CUSTOM_HID_HandleTypeDef *hhid = (USBD_CUSTOM_HID_HandleTypeDef*)hUsbDeviceFS.pClassData;

  DBG_LED1_ON();

  Request left_sensor_req;
  Request down_sensor_req;
  Request up_sensor_req;
  Request right_sensor_req;

  // This builds requests that instruct to store sensor data in sensor_buffer
  // in relevant locations
  BUILD_SENSOR_REQ(left_sensor_req, COMPORT_LEFT);
  BUILD_SENSOR_REQ(down_sensor_req, COMPORT_DOWN);
  BUILD_SENSOR_REQ(up_sensor_req, COMPORT_UP);
  BUILD_SENSOR_REQ(right_sensor_req, COMPORT_RIGHT);

  //uint8_t sensors_collected = 0x0;
  msgbus_send_request(left_sensor_req);
  msgbus_send_request(down_sensor_req);
  msgbus_send_request(up_sensor_req);
  msgbus_send_request(right_sensor_req);

  uint8_t have_new_data = false;

  while (1) {
    msgbus_process_flags();

    if (msgbus_have_pending_response()) {
      Response * resp = msgbus_get_pending_response();

      if (resp->request_command == CMD_REQUEST_SENSORS) {
        // Flip a bit on when we've collected a sensor's data
        //sensors_collected |= (1 << (uint8_t)resp->comport_id);
        DBG_LED2_TOGGLE();
        uint8_t offset = ((uint8_t)resp->comport_id) * 8;
        for (uint8_t i = 0; i < 8; i++) {
          usb_sensor_buffer[offset + i] = sensor_buffer[offset + i];
        }

        //sensors_collected |= (1 << resp->comport_id);

        have_new_data = true;
      }

      // TODO: deal with responses to other sorts fo commands here

    }

    DBG_LED1_TOGGLE();
    USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, usb_sensor_buffer, USB_HID_PACKET_SIZE_BYTES);
    msgbus_send_request(left_sensor_req);
    msgbus_send_request(down_sensor_req);
    msgbus_send_request(up_sensor_req);
    msgbus_send_request(right_sensor_req);


    // TODO: also receive USB here?

    /*cmd = CMD_SEND_LED_DATA;
    // Left
    HAL_UART_Transmit(&huart1_l, &cmd, 1, 100);
    HAL_UART_Transmit(&huart1_l, sensor_buffer, 64, 100);
    cmd = send_lights;
    HAL_UART_Transmit(&huart1_l, &cmd, 1, 100);*/
    /*if(segments_received == 0xFFFF){
      cmd = CMD_COMMIT_LED_DATA;
      // Left
      HAL_UART_Transmit(&huart1_l, &cmd, 1, 1);
      
      // Up
      usart_switch(USART_UP);
      HAL_UART_Transmit(&huart2_u_r, &cmd, 1, 1);
      
      // Right
      usart_switch(USART_RIGHT);
      HAL_UART_Transmit(&huart2_u_r, &cmd, 1, 1);
      
      // Down
      HAL_UART_Transmit(&huart3_d, &cmd, 1, 1);
      segments_received = 0x0000;
    }

    if(packet_received){
      led_info = hhid->Report_buf[0];
      uint8_t panel = (led_info >> 6) & 0x03;
      uint8_t segment = (led_info >> 4) & 0x03;
      uint8_t frame = led_info & 0x07;
    
      if(frame != last_frame){
        segments_received = 0x0000;
      }
    
      last_frame = frame;
      segments_received |= 1 << (panel * 4 + segment);
    
      if(panel == 0x00){
        cmd = send_led_data;
        HAL_UART_Transmit(&huart1_l, &cmd, 1, 1);
        HAL_UART_Transmit(&huart1_l, hhid->Report_buf, 64, 2);
      }
    
      if(panel == 0x01){
        cmd = send_led_data;
        HAL_UART_Transmit(&huart3_d, &cmd, 1, 1);
        HAL_UART_Transmit(&huart3_d, hhid->Report_buf, 64, 2);
      }
    
      if(panel == 0x02){
        usart_switch(USART_UP);
        cmd = send_led_data;
        HAL_UART_Transmit(&huart2_u_r, &cmd, 1, 1);
        HAL_UART_Transmit(&huart2_u_r, hhid->Report_buf, 64, 2);
      }
    
      if(panel == 0x03){
        usart_switch(USART_RIGHT);
        cmd = send_led_data;
        HAL_UART_Transmit(&huart2_u_r, &cmd, 1, 1);
        HAL_UART_Transmit(&huart2_u_r, hhid->Report_buf, 64, 2);
      }
    }*/
  }
}

void SystemClock_Config(void){
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK){
    Error_Handler(1000);
  }
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK){
    Error_Handler(1000);
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_USART1
                              |RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_USART3;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
  PeriphClkInit.USBClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK){
    Error_Handler(1000);
  }
}

static void MX_GPIO_Init(void){
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  // -- Write relevant GPIO pins LOW

  // A 0,1,6,7: Debug pins
  HAL_GPIO_WritePin(GPIOA, 
      GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_6 | GPIO_PIN_7,
      GPIO_PIN_RESET);

  // C 13,14,15: Status LEDs
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  // -- configure pin modes
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  // A 0,1,6,7: Debug Pins (outputs)
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // C 13,14,15: Status LEDS (outputs)
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

#ifdef  USE_FULL_ASSERT
void assert_failed(char *file, uint32_t line){ 
}
#endif
