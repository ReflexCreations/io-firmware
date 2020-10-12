#include "main.h"
#include "usb_device.h"
#include "usbd_customhid.h"

#define DBG_LED1_ON() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET)
#define DBG_LED1_OFF() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET)
#define DBG_LED1_TOGGLE() HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13)

#define DBG_LED2_ON() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_SET)
#define DBG_LED2_OFF() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_RESET)
#define DBG_LED2_TOGGLE() HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_14)

#define DBG_LED3_ON() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_SET)
#define DBG_LED3_OFF() HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_RESET)
#define DBG_LED3_TOGGLE() HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_15)

extern USBD_HandleTypeDef hUsbDeviceFS;

UART_HandleTypeDef huart1; // Left
UART_HandleTypeDef huart2; // Up, Right
UART_HandleTypeDef huart3; // Down
DMA_HandleTypeDef hdma_usart1_rx; // Left
DMA_HandleTypeDef hdma_usart1_tx;
DMA_HandleTypeDef hdma_usart2_rx; // Up, right
DMA_HandleTypeDef hdma_usart2_tx;
DMA_HandleTypeDef hdma_usart3_rx; // Down
DMA_HandleTypeDef hdma_usart3_tx;

extern uint8_t packet_received;

typedef enum {
  UART_UP,
  UART_RIGHT
} UartToggle;

typedef enum {
  CMD_REQUEST_SENSORS = 0x01,
  CMD_TRANSMIT_LED_DATA = 0x02,
  CMD_COMMIT_LEDS = 0x03
} Commands;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART_INIT_generic(UART_HandleTypeDef *, USART_TypeDef *, IRQn_Type);
static void MX_USART2_UART_Init(void);
void startup_sequence(void);
void usart_switch(UartToggle);

uint8_t switch_ready = 0;

int main(void){
  HAL_Init();
  MX_GPIO_Init();
  SystemClock_Config();
  startup_sequence();
  MX_USB_DEVICE_Init();
  MX_DMA_Init();

  uint8_t left = 0x00;
  uint8_t down = 0x01;
  uint8_t up = 0x02;
  uint8_t right = 0x03;
  uint8_t up_left = 0x00;
  uint8_t up_right = 0x01;
  uint8_t down_left = 0x02;
  uint8_t down_right = 0x03;
  uint8_t request_sensors = 0x10;
  uint8_t send_lights = 0x30;
  uint8_t send_led_data = 0x20;
  uint8_t sensor_buffer[64];
  uint8_t led_info = 0x00;
  uint16_t segments_received = 0x0000;
  uint8_t last_frame = 0x00;

  USBD_CUSTOM_HID_HandleTypeDef *hhid = (USBD_CUSTOM_HID_HandleTypeDef*)hUsbDeviceFS.pClassData;

  DBG_LED1_ON();

  while (1){
    uint8_t i = 0;
    uint8_t cmd = 0;
    for(; i < 64; i++){
      sensor_buffer[i] = 0;
    } 
    /*cmd = ((uint8_t)CMD_SEND_LED_DATA) << 8;
    // Left
    HAL_UART_Transmit(&huart1, &cmd, 1, 100);
    HAL_UART_Transmit(&huart1, sensor_buffer, 64, 100);
    cmd = send_lights;
    HAL_UART_Transmit(&huart1, &cmd, 1, 100);*/
    /*if(segments_received == 0xFFFF){
      cmd = send_lights;
      // Left
      HAL_UART_Transmit(&huart1, &cmd, 1, 1);
      
      // Up
      usart_switch(USART_UP);
      HAL_UART_Transmit(&huart2, &cmd, 1, 1);
      
      // Right
      usart_switch(USART_RIGHT);
      HAL_UART_Transmit(&huart2, &cmd, 1, 1);
      
      // Down
      HAL_UART_Transmit(&huart3, &cmd, 1, 1);
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
        HAL_UART_Transmit(&huart1, &cmd, 1, 1);
        HAL_UART_Transmit(&huart1, hhid->Report_buf, 64, 2);
      }
    
      if(panel == 0x01){
        cmd = send_led_data;
        HAL_UART_Transmit(&huart3, &cmd, 1, 1);
        HAL_UART_Transmit(&huart3, hhid->Report_buf, 64, 2);
      }
    
      if(panel == 0x02){
        usart_switch(USART_UP);
        cmd = send_led_data;
        HAL_UART_Transmit(&huart2, &cmd, 1, 1);
        HAL_UART_Transmit(&huart2, hhid->Report_buf, 64, 2);
      }
    
      if(panel == 0x03){
        usart_switch(USART_RIGHT);
        cmd = send_led_data;
        HAL_UART_Transmit(&huart2, &cmd, 1, 1);
        HAL_UART_Transmit(&huart2, hhid->Report_buf, 64, 2);
      }
    }*/
      
    cmd = request_sensors;
    // Mich note: timeouts are in multiples of systick.
    // This is set to 1ms, so Timeout=1 == 1ms
    
    // Left
    HAL_UART_Transmit(&huart1, &cmd, 1, 3);

    if (HAL_UART_Receive(&huart1, sensor_buffer, 8, 30) == HAL_OK) {
      DBG_LED2_ON();
      DBG_LED3_OFF();
    } else {
      DBG_LED2_OFF();
      DBG_LED3_ON();
    }

    // Up
    //usart_switch(UART_UP);
    //HAL_UART_Transmit(&huart2, &cmd, 1, 1);
    //HAL_UART_Receive(&huart2, sensor_buffer + 16, 8, 1);
    
    // Down
    //HAL_UART_Transmit(&huart3, &cmd, 1, 1);
    //HAL_UART_Receive(&huart3, sensor_buffer + 8, 8, 1);
    
    // Right
    //usart_switch(UART_RIGHT);
    //HAL_UART_Transmit(&huart2, &cmd, 1, 1);
    //HAL_UART_Receive(&huart2, sensor_buffer + 24, 8, 1);
    
    USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, sensor_buffer, 64);
  }
}

void USART1_IRQHandler(){
  HAL_UART_IRQHandler(&huart1);
}

void USART2_IRQHandler(){
  switch_ready = 1;
  HAL_UART_IRQHandler(&huart2);
}

void USART3_IRQHandler(){
  HAL_UART_IRQHandler(&huart3);
}

void startup_sequence(void){
  GPIO_InitTypeDef startup_gpio = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  //Set up GPIO input direction for RS-485 during start-up and USART directions on transceivers.
  startup_gpio.Pin = GPIO_PIN_2 | GPIO_PIN_7 | GPIO_PIN_9 | GPIO_PIN_13;
  startup_gpio.Pin |= GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_8 | GPIO_PIN_14 | GPIO_PIN_15;
  startup_gpio.Mode = GPIO_MODE_OUTPUT_PP;
  startup_gpio.Pull = GPIO_NOPULL;
  startup_gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &startup_gpio);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2 | GPIO_PIN_7 | GPIO_PIN_9 | GPIO_PIN_13, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_6 | GPIO_PIN_14, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_8 | GPIO_PIN_15, GPIO_PIN_RESET);
  startup_gpio.Pin = GPIO_PIN_5;
  HAL_GPIO_Init(GPIOA, &startup_gpio);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
  //Set up GPIO pins for board ready state.
  startup_gpio.Pin = GPIO_PIN_5 | GPIO_PIN_12;
  startup_gpio.Mode = GPIO_MODE_INPUT;
  HAL_GPIO_Init(GPIOB, &startup_gpio);
  startup_gpio.Pin = GPIO_PIN_4 | GPIO_PIN_8;
  startup_gpio.Mode = GPIO_MODE_INPUT;
  HAL_GPIO_Init(GPIOA, &startup_gpio);
  //Wait until all boards have asserted high.
  while(!HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8)){} // LEFT
  //while(!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12)){} // DOWN
  //while(!HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4)){} // UP
  //while(!HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5)){} // RIGHT
  //Set up USART modes

  MX_USART_INIT_generic(&huart1, USART1, USART1_IRQn);
  //MX_USART2_UART_Init();
  //MX_USART_INIT_generic(&huart3, USART3, USART3_IRQn);
}

void usart_switch(UartToggle which) {
  HAL_UART_MspDeInit(&huart2);
  HAL_UART_DeInit(&huart2);
  GPIO_InitTypeDef usart_gpio = {0};

  if (which == UART_UP) {
    usart_gpio.Pin = GPIO_PIN_15;
    usart_gpio.Mode = GPIO_MODE_ANALOG;
    usart_gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &usart_gpio);
    usart_gpio.Pin = GPIO_PIN_3;
    HAL_GPIO_Init(GPIOB, &usart_gpio);
    usart_gpio.Pin =GPIO_PIN_2 | GPIO_PIN_3;
    usart_gpio.Mode = GPIO_MODE_AF_PP;
    usart_gpio.Pull = GPIO_NOPULL;
    usart_gpio.Alternate = GPIO_AF7_USART2;
    usart_gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &usart_gpio);
  } else { // Right
    usart_gpio.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    usart_gpio.Mode = GPIO_MODE_ANALOG;
    usart_gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &usart_gpio);
    usart_gpio.Pin = GPIO_PIN_15;
    usart_gpio.Mode = GPIO_MODE_AF_PP;
    usart_gpio.Pull = GPIO_NOPULL;
    usart_gpio.Alternate = GPIO_AF7_USART2;
    usart_gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &usart_gpio);
    usart_gpio.Pin = GPIO_PIN_3;
    HAL_GPIO_Init(GPIOB, &usart_gpio);
  }

  MX_USART2_UART_Init();
  HAL_UART_MspInit(&huart2);
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
    Error_Handler();
  }
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK){
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB|RCC_PERIPHCLK_USART1
                              |RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_USART3;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
  PeriphClkInit.USBClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK){
    Error_Handler();
  }
}

static void MX_USART2_UART_Init(void)
{
  MX_USART_INIT_generic(&huart2, USART2, USART2_IRQn);
}

static void MX_USART_INIT_generic(
  UART_HandleTypeDef *huart, USART_TypeDef *usart, IRQn_Type irqn)
{
  huart->Instance = usart;
  huart->Init.BaudRate = 512000;
  huart->Init.WordLength = UART_WORDLENGTH_8B;
  huart->Init.StopBits = UART_STOPBITS_2;
  huart->Init.Parity = UART_PARITY_NONE;
  huart->Init.Mode = UART_MODE_TX_RX;
  huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart->Init.OverSampling = UART_OVERSAMPLING_16;
  huart->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  HAL_NVIC_SetPriority(irqn, 0, 0);
  HAL_NVIC_EnableIRQ(irqn);
  if (HAL_UART_Init(huart) != HAL_OK){
    Error_Handler();
  }
}

static void MX_DMA_Init(void) {
  __HAL_RCC_DMA1_CLK_ENABLE();
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);

  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);

  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);

  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);

  HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);
}

static void MX_GPIO_Init(void){
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_5|GPIO_PIN_6 
                          |GPIO_PIN_7, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_13 
                          |GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_4|GPIO_PIN_6 
                          |GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_5|GPIO_PIN_6 
                          |GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_13 
                          |GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_4|GPIO_PIN_6 
                          |GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void Error_Handler(void){

}

#ifdef  USE_FULL_ASSERT
void assert_failed(char *file, uint32_t line){ 
}
#endif
