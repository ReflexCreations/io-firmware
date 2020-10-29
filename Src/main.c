#include "main.h"
#include "usb_device.h"
#include "usbd_customhid.h"
#include "bool.h"
#include "uart.h"
#include "commands.h"
#include "msgbus.h"
#include "debug_leds.h"
#include "error_handler.h"
#include "commtests.h"
#include "ledtests.h"

#define USB_HID_PACKET_SIZE_BYTES (64U)
#define BYTES_PER_SEGMENT (64U)
#define SEGMENTS_PER_PANEL (4U)
#define BYTES_PER_PANEL (BYTES_PER_SEGMENT * SEGMENTS_PER_PANEL)
#define PANELS_PER_PLATFORM (4U)
#define LED_ARRAY_SIZE (BYTES_PER_PANEL * PANELS_PER_PLATFORM)

#define COMPLETE_FRAME (0xFFFF)

#define BUILD_SENSOR_REQ(req, comport) do { \
  req.comport_id = comport; \
  req.request_command = Command_Request_Sensors; \
  req.send_data = NULL; \
  req.send_data_len = 0; \
  req.response_data = sensor_buffer + ((uint8_t)comport * 8); \
  req.response_len = 8; \
} while (0U);
  

#define BUILD_TRANSMIT_LEDS_REQ(req, comport, segment) do {\
  req.comport_id = comport; \
  req.request_command = Command_Process_LED_Segment; \
  req.send_data = led_buffer + \
    (comport * BYTES_PER_PANEL) + (segment * BYTES_PER_SEGMENT); \
  req.send_data_len = BYTES_PER_SEGMENT; \
  req.response_data = NULL; \
  req.response_len = 0; \
} while (0U);

#define BUILD_COMMIT_LEDS_REQ(req, comport) do { \
  req.comport_id = comport; \
  req.request_command = Command_Commit_LEDs; \
  req.send_data = NULL; \
  req.send_data_len = 0; \
  req.response_data = NULL; \
  req.response_len = 0; \
} while (0U);

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
uint8_t led_buffer[LED_ARRAY_SIZE];

uint8_t test_buffer_2b[2];
uint8_t test_buffer_64b[64];

uint16_t segments_received = 0x0000;

uint8_t last_packet_header = 0x00;

extern uint8_t rep_buf[USB_HID_PACKET_SIZE_BYTES];

extern bool packet_received;


void SystemClock_Config(void);
static void MX_GPIO_Init(void);

// Seems that when sending LED packets, it sends broken data to 2 ports,
// and tells them to commit. Usually comes up all white. Then everything breaks,
// there's no more new sensor data received from panel boards. Seems same buffer
// keeps getting sent.

// Need to somehow debug this. Maybe parity check should be enable to check
// data integrity now we're sending so much, so fast?


int main(void){
  HAL_Init();
  MX_GPIO_Init();
  SystemClock_Config();
  uart_init();
  msgbus_init();
  
  MX_USB_DEVICE_Init();

  uint8_t packet_header = 0x00;
  uint8_t previous_frame = 0x00;

  USBD_CUSTOM_HID_HandleTypeDef *hhid = \
      (USBD_CUSTOM_HID_HandleTypeDef*)hUsbDeviceFS.pClassData;

  DBG_LED1_ON();

  Request left_sensor_req;
  Request down_sensor_req;
  Request up_sensor_req;
  Request right_sensor_req;

  Request transmit_leds_req;

  // This builds requests that instruct to store sensor data in sensor_buffer
  // in relevant locations
  BUILD_SENSOR_REQ(left_sensor_req, Comport_Left);
  BUILD_SENSOR_REQ(down_sensor_req, Comport_Down);
  BUILD_SENSOR_REQ(up_sensor_req, Comport_Up);
  BUILD_SENSOR_REQ(right_sensor_req, Comport_Right);

  //msgbus_send_request(left_sensor_req);
  //msgbus_send_request(down_sensor_req);
  //msgbus_send_request(up_sensor_req);
  //msgbus_send_request(right_sensor_req);

  ComportId port_1 = Comport_Down;
  ComportId port_2 = Comport_Right;

  if (commtest_dual_receive_2bytes(port_1, port_2)
      && commtest_dual_receive_64bytes(port_1, port_2)
      && commtest_dual_double_values(port_1, port_2)) {
    DBG_LED3_ON();
  } else {
    DBG_LED3_OFF();
  }

  ledtests_hardcoded_LEDs(Comport_Right);
  msgbus_wait_for_idle(Comport_Right);

  DBG_LED3_OFF();

  //HAL_Delay(1000);


  if (commtest_receive_2bytes(Comport_Right)) {
    DBG_LED3_ON();
  } else {
    DBG_LED3_OFF();
  }

  uint8_t max_br = 0x08;
  uint16_t frame_ms = 50;
  while (1) {
    for (uint8_t i = 0; i < max_br; i++) { // R: 255->0, G: 0->255, B: 0
      HAL_Delay(frame_ms);
      ledtests_solid_color_LEDs(Comport_Right, max_br - i, i, 0x00);
      msgbus_wait_for_idle(Comport_Right);
    }
    for (uint8_t i = 0; i < max_br; i++) { // R: 0, G: 255->0, B 0->255
      HAL_Delay(frame_ms);
      ledtests_solid_color_LEDs(Comport_Right, 0, max_br - i, i);
      msgbus_wait_for_idle(Comport_Right);
    }
    for (uint8_t i = 0; i < max_br; i++) { // R 0->255, G: 0, B: 255->0
      HAL_Delay(frame_ms);
      ledtests_solid_color_LEDs(Comport_Right, i, 0, max_br - i);
      msgbus_wait_for_idle(Comport_Right);
    }
  }

  // TODO: a test for sending a full buffer of LED data from IO board

  while(1) {
    msgbus_process_flags();
  }

  while (1) {
    msgbus_process_flags();

    if (msgbus_have_pending_response()) {
      DBG_LED1_TOGGLE();
      Response * resp = msgbus_get_pending_response();

      if (resp->request_command == Command_Request_Sensors) {
        //DBG_LED2_TOGGLE();
        uint8_t offset = ((uint8_t)resp->comport_id) * 8;
        for (uint8_t i = 0; i < 8; i++) {
          usb_sensor_buffer[offset + i] = sensor_buffer[offset + i];
        }
      }

      if (resp->request_command == Command_Test_Expect_2B) {
        if (test_buffer_2b[0] == 0xBE && test_buffer_2b[1] == 0xEF) {
          DBG_LED3_ON();
        }
      }

      // TODO: deal with responses to other sorts of commands here

    }
  }

  while(0) {

    // -------------------------------------
    // TODO current issue to investigate
    //
    // it seems Command_Commit_LEDs is never being sent, at least not received
    // on the panel side according to debug LEDS.
    // Find out if on here we think we're sending it with debug LED
    // If not, well, figure out why not.
    // Also it's running at <1000Hz, so any optimizations, go.
    // Also fucking go to bed on time jesus christ it's 1:15 am on a monday
    //

    USBD_CUSTOM_HID_SendReport(
      &hUsbDeviceFS, usb_sensor_buffer, USB_HID_PACKET_SIZE_BYTES
    );

    if (segments_received == COMPLETE_FRAME) {
      Request commit_req;
      BUILD_COMMIT_LEDS_REQ(commit_req, Comport_Left);

      msgbus_send_request(commit_req);
      commit_req.comport_id = Comport_Down;
      msgbus_send_request(commit_req);
      commit_req.comport_id = Comport_Up;
      msgbus_send_request(commit_req);
      //commit_req.comport_id = COMPORT_RIGHT;
      //msgbus_send_request(commit_req);
      //DBG_LED3_ON();

      segments_received = 0x0000;
    }

    if (packet_received) {
      //DBG_LED3_TOGGLE();
      packet_header = rep_buf[0];
      last_packet_header = packet_header;
      // Not sure the enum type serves any purpose here actually
      uint8_t panel = (packet_header >> 6) & 0x03;
      uint8_t segment = (packet_header >> 4) & 0x03;
      uint8_t frame = packet_header & 0x0F;

      uint16_t buffer_offset = 
          panel * BYTES_PER_PANEL + segment * BYTES_PER_SEGMENT;

      for (uint8_t i = 0; i < USB_HID_PACKET_SIZE_BYTES; i++) {
        led_buffer[i + buffer_offset] = rep_buf[i];
      }

      if (frame != previous_frame) {
        //segments_received = 0x0000;
      }

      previous_frame = frame;
      segments_received |= (1 << (panel * PANELS_PER_PLATFORM + segment));

      BUILD_TRANSMIT_LEDS_REQ(transmit_leds_req, (ComportId)panel, segment);
      msgbus_send_request(transmit_leds_req);
      packet_received = false;
    }

    msgbus_send_request(left_sensor_req);
    msgbus_send_request(down_sensor_req);
    msgbus_send_request(up_sensor_req);
    //msgbus_send_request(right_sensor_req);
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
