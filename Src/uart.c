#include "uart.h"
#include "bool.h"
#include "error_handler.h"
#include "config.h"

#define RS485_CK_DR_PINS_B (GPIO_PIN_2 | GPIO_PIN_7 | GPIO_PIN_9 | GPIO_PIN_13)
#define RS485_TX_DR_PINS_B (GPIO_PIN_0 | GPIO_PIN_14 | GPIO_PIN_6)
#define RS485_TX_DR_PINS_A (GPIO_PIN_5)
#define RS485_RX_DR_PINS_B (GPIO_PIN_1 | GPIO_PIN_15 | GPIO_PIN_4 | GPIO_PIN_8)

#define RS485_CK_PINS_A (GPIO_PIN_4 | GPIO_PIN_8)
#define RS485_CK_PINS_B (GPIO_PIN_5 | GPIO_PIN_12)

// GPIO pins to configure for alternate function USART2 to
// hooked the peripheral up to the "up" panel connector
#define UART2_UP_PINS_A (GPIO_PIN_2 | GPIO_PIN_3)
#define UART2_UP_PINS_B (0)
// And the same for the "right" panel connector
#define UART2_RIGHT_PINS_A GPIO_PIN_15
#define UART2_RIGHT_PINS_B GPIO_PIN_3

#define PANEL_LEFT_INITIALIZED (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8))
#define PANEL_UP_INITIALIZED (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4))
#define PANEL_DOWN_INITIALIZED (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12))
#define PANEL_RIGHT_INITIALIZED (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5))

#define ALL_INITIALIZED (\
    (!PANEL_LEFT_CONNECTED || PANEL_LEFT_INITIALIZED) && \
    (!PANEL_UP_CONNECTED || PANEL_UP_INITIALIZED) && \
    (!PANEL_DOWN_CONNECTED || PANEL_DOWN_INITIALIZED) && \
    (!PANEL_RIGHT_CONNECTED || PANEL_DOWN_INITIALIZED) \
)

/**
 * Notes regarding UART / USART
 * 
 * Initially the boards were designed to use USART (so including a clock),
 * but it appears that the controller used on the panel boards is unable to
 * run in "slave" usart mode. Thus, the clock line referred to as CK is largely
 * unused, except for waiting for panel boards to indicate they are ready, at
 * which time they'll write this line high.
 */

UART_HandleTypeDef huart1_l; // Left
UART_HandleTypeDef huart2_u_r; // Up, Right
UART_HandleTypeDef huart3_d; // Down

DMA_HandleTypeDef hdma_usart1_l_rx; // Left
DMA_HandleTypeDef hdma_usart1_l_tx;
DMA_HandleTypeDef hdma_usart2_u_r_rx; // Up, right
DMA_HandleTypeDef hdma_usart2_u_r_tx;
DMA_HandleTypeDef hdma_usart3_d_rx; // Down
DMA_HandleTypeDef hdma_usart3_d_tx;

// up or right connector, this indicates which one is configured on USART2
ComportId switched_comport = Comport_None;

static SendCompleteHandler send_complete_handler = NULL;
static ReceiveCompleteHandler receive_complete_handler = NULL;

static void init_gpio();
static void init_rs485();
static void init_periph(UART_HandleTypeDef *, USART_TypeDef *, IRQn_Type);
static void init_dma_interrupts();

static UART_HandleTypeDef * get_uart_handle(ComportId);

static UART_HandleTypeDef * uart_handles[COMPORT_ID_MAX + 1];

static inline UART_HandleTypeDef * get_uart_handle(ComportId comport_id) {
    if (comport_id > COMPORT_ID_MAX) {
        error_panic_data(Error_App_UART_InvalidComport, comport_id);
    }

    return uart_handles[comport_id];
}

void uart_init() {
    init_gpio();
    init_rs485();
    init_dma_interrupts();

    // Left
    init_periph(&huart1_l, USART1, USART1_IRQn);
    // Up, Right
    init_periph(&huart2_u_r, USART2, USART2_IRQn);
    // Down
    init_periph(&huart3_d, USART3, USART3_IRQn);

    uart_handles[Comport_Left] = &huart1_l;
    uart_handles[Comport_Down] = &huart3_d;
    uart_handles[Comport_Up] = &huart2_u_r;
    uart_handles[Comport_Right] = &huart2_u_r;

    // Wait for all panel boards marked as connected to signal their readiness
    while (!ALL_INITIALIZED);
}

void uart_send(ComportId comport_id, uint8_t * data_ptr, uint16_t data_len) {
    UART_HandleTypeDef * huart = get_uart_handle(comport_id);
    HAL_StatusTypeDef result = HAL_UART_Transmit_DMA(huart, data_ptr, data_len);
    
    if (result != HAL_OK) {
        error_panic_data(Error_HAL_UART_Transmit_DMA, (uint32_t)result);
    }
}

void uart_receive(ComportId comport_id, uint8_t * data_ptr, uint16_t data_len) {
    UART_HandleTypeDef * huart = get_uart_handle(comport_id);
    HAL_StatusTypeDef result = HAL_UART_Receive_DMA(huart, data_ptr, data_len);
    
    if (result != HAL_OK) {
        error_panic_data(Error_HAL_UART_Receive_DMA, result);
    }
}

void uart_abort_receive(ComportId comport_id) {
    HAL_UART_AbortReceive(get_uart_handle(comport_id));
}

void uart_set_on_send_complete_handler(SendCompleteHandler handler) {
    send_complete_handler = handler;
}

void uart_set_on_receive_complete_handler(ReceiveCompleteHandler handler) {
    receive_complete_handler = handler;
}

void uart_connect_port(ComportId comport_id) {
    // Only right/up need any special work, so others are no-op.
    if (comport_id != Comport_Right && comport_id != Comport_Up) return;

    // Don't do anything if the switched uart already matches this one
    if (switched_comport == comport_id) return;

    HAL_NVIC_DisableIRQ(USART2_IRQn);

    HAL_UART_MspDeInit(&huart2_u_r);
    
    HAL_StatusTypeDef de_init_result = HAL_UART_DeInit(&huart2_u_r);

    if (de_init_result != HAL_OK) {
        error_panic_data(Error_HAL_UART_DeInit, de_init_result);
    }
    
    uint32_t af_pins_a, af_pins_b;
    uint32_t float_pins_a, float_pins_b;

    uint8_t is_up = comport_id == Comport_Up;

    af_pins_a = is_up ? UART2_UP_PINS_A : UART2_RIGHT_PINS_A;
    af_pins_b = is_up ? UART2_UP_PINS_B : UART2_RIGHT_PINS_B;
    float_pins_a = is_up ? UART2_RIGHT_PINS_A : UART2_UP_PINS_A;
    float_pins_b = is_up ? UART2_RIGHT_PINS_B : UART2_UP_PINS_B;

    GPIO_InitTypeDef gpio = {0};

    // Disconnect one connector
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;

    if (float_pins_a != 0x00) {
        gpio.Pin = float_pins_a;
        HAL_GPIO_Init(GPIOA, &gpio);
    }

    if (float_pins_b != 0x00) {
        gpio.Pin = float_pins_b;
        HAL_GPIO_Init(GPIOB, &gpio);
    }

    // Connect the other, configuring for alternate function
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Alternate = GPIO_AF7_USART2;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    if (af_pins_a != 0x00) {
        gpio.Pin = af_pins_a;
        HAL_GPIO_Init(GPIOA, &gpio);
    }

    if (af_pins_b != 0x00) {
        gpio.Pin = af_pins_b;
        HAL_GPIO_Init(GPIOB, &gpio);
    }
    
    init_periph(&huart2_u_r, USART2, USART2_IRQn);
    HAL_UART_MspInit(&huart2_u_r);
    switched_comport = comport_id;
}

// Initialization

static void init_gpio() {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // -- Write relevant GPIO pins LOW

    // A 5: USART2-0_TX, RS485 output enable ("up") - see also B8, B9
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

    // B 0,1,2: USART3, RS485 output enables ("down")
    // B 13,14,15: USART1, RS485 output enables ("left")
    // B 4,6,7: USART2-1, RS485 output enables ("right")
    // B 8,9: USART2-0, RS485 output enables ("up") - see also A5
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_13 
                            |GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_4|GPIO_PIN_6 
                            |GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9, GPIO_PIN_RESET);

    // -- configure pin modes

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // A 5: USART2-0 TX; RS485 output enable ("up") - see also B8, B9
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // A 4: USART2-0_CK;, Clock input for "up" -- used for wait init (input)
    // A 8: USART1_CK, Clock input for "left" -- used for wait init (input)
    GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // B 0,1,2:    USART3   TX,RX,CK; RS485 output enables ("down") (outputs)
    // B 13,14,15: USART1   CK,TX,RX; RS485 output enables ("left") (outputs)
    // B 4,6,7:    USART2-1 RX,TX,CK; RS485 output enables ("right") (outputs)
    // B 8,9:      USART2-0 RX,CK;    RS485 output enables ("up") - see also A5 (outputs)
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_13 
                            |GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_4|GPIO_PIN_6 
                            |GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // B 12: USART3_CK, Clock input for "down" -- used for wait init (input)
    // B 5: USART2-1_CK, Clock input for "right" -- used for wait init (input)
    GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

static void init_rs485() {
    // Init pin config
    GPIO_InitTypeDef gpio = {0};
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    gpio.Pin = RS485_TX_DR_PINS_A;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = RS485_CK_DR_PINS_B | RS485_TX_DR_PINS_B | RS485_RX_DR_PINS_B;
    HAL_GPIO_Init(GPIOB, &gpio);

    // Write output and input directions to RS485 transceivers
    // TX, configured as output
    HAL_GPIO_WritePin(GPIOA, RS485_TX_DR_PINS_A, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, RS485_TX_DR_PINS_B, GPIO_PIN_SET);
    // RX, configured as input
    HAL_GPIO_WritePin(GPIOB, RS485_RX_DR_PINS_B, GPIO_PIN_RESET);
    // CK (clock), unused, configured as input
    HAL_GPIO_WritePin(GPIOB, RS485_CK_DR_PINS_B, GPIO_PIN_RESET);
}

static void init_periph(
    UART_HandleTypeDef *huart, USART_TypeDef *usart, IRQn_Type irqn) {

    huart->Instance = usart;
    huart->Init.BaudRate = 3000000;
    huart->Init.WordLength = UART_WORDLENGTH_8B;
    huart->Init.StopBits = UART_STOPBITS_2;
    huart->Init.Parity = UART_PARITY_NONE;
    huart->Init.Mode = UART_MODE_TX_RX;
    huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart->Init.OverSampling = UART_OVERSAMPLING_8;
    huart->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE; // DIS
    huart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    HAL_NVIC_SetPriority(irqn, 0, 0);
    HAL_NVIC_EnableIRQ(irqn);

    if (HAL_UART_Init(huart) != HAL_OK){
        error_panic_data(Error_HAL_UART_Init, (uint32_t)usart);
    }
}

static void init_dma_interrupts() {
    __HAL_RCC_DMA1_CLK_ENABLE();

    // USART3_TX (Down)
    HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);

    // USART3_RX (Down) 
    HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);

    // USART1_TX (Left)
    HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);

    // USART1_RX (Left)
    HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

    // USART2_RX (Up, Right)
    HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);

    // USART2_TX (Up, Right)
    HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (send_complete_handler == NULL) return;

    if (huart == &huart1_l) {
        send_complete_handler(Comport_Left);
    } else if (huart == &huart2_u_r) {
        send_complete_handler(switched_comport);
    } else {
        send_complete_handler(Comport_Down);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (receive_complete_handler == NULL) return;

    if (huart == &huart1_l) {
        receive_complete_handler(Comport_Left);
    } else if (huart == &huart2_u_r) {
        receive_complete_handler(switched_comport);
    } else {
        receive_complete_handler(Comport_Down);
    }
}