/*
 * switches.c
 *
 *  Created on: Jul 6, 2026
 *      Author: Luke Fadel
 */

#include "switches.h"
#include "main.h"
#include "stm32f0xx_hal_can.h"
#include "stm32f0xx_hal_def.h"
#include "stm32f0xx_hal_uart.h"

// global array to track switch state
// arrays 1-3 are for the last 3 states, array 4 is for the debounced state
// current index pointer loops circularly through arrays 1-3
static uint8_t switchState[4][5];
static uint8_t currentIdx;
static CAN_HandleTypeDef *can;
// for debugging
static UART_HandleTypeDef *uart;

// initializing function that sets the can device to use in the interrupt
HAL_StatusTypeDef SWITCHES_Init(CAN_HandleTypeDef *canInterface,
                                UART_HandleTypeDef *uartInterface) {
  can = canInterface;
  uart = uartInterface;
  return HAL_OK;
}

// reads the state of switches. Run in main loop
HAL_StatusTypeDef SWITCHES_Process(CAN_HandleTypeDef *canInterface) {
  // reading current switch state
  switchState[currentIdx][0] = HAL_GPIO_ReadPin(SWITCH1_GPIO_Port, SWITCH1_Pin);
  switchState[currentIdx][1] = HAL_GPIO_ReadPin(SWITCH2_GPIO_Port, SWITCH2_Pin);
  switchState[currentIdx][2] = HAL_GPIO_ReadPin(SWITCH3_GPIO_Port, SWITCH3_Pin);
  switchState[currentIdx][3] = HAL_GPIO_ReadPin(SWITCH4_GPIO_Port, SWITCH4_Pin);
  switchState[currentIdx][4] = HAL_GPIO_ReadPin(SWITCH5_GPIO_Port, SWITCH5_Pin);

  // incrementing circular index
  currentIdx = (currentIdx + 1) % 3;

  return HAL_OK;
}

// on interrupt, transmit switch state
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim->Instance == TIM2) {

    // debouncing switches
    uint8_t debouncedState = 0;

    // iterating through last 3 states
    for (uint8_t sw = 0; sw < 5; sw++) {
      // computing debounced state
      switchState[3][sw] = (switchState[0][sw] == switchState[1][sw] &&
                            switchState[1][sw] == switchState[2][sw])
                               ? switchState[0][sw]
                               : switchState[3][sw];

      // packing payload into specified format to send
      debouncedState |= switchState[3][sw] << sw;
    }

    // transmitting debounced state
    CAN_TxHeaderTypeDef header = {0};

    // 1 byte of data, 1 bit per switch
    header.DLC = 1;

    // as specified in protocol
    header.StdId = 0x140;

    // can msg settings
    header.IDE = CAN_ID_STD;
    header.RTR = CAN_RTR_DATA;
    header.TransmitGlobalTime = DISABLE;

    uint32_t mailbox;

    HAL_CAN_AddTxMessage(can, &header, &debouncedState, &mailbox);
  }
}
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan) {
  HAL_UART_Transmit_IT(uart, (const uint8_t *)"completed switch transmission\n",
                       30);
}
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan) {
  HAL_UART_Transmit_IT(uart, (const uint8_t *)"completed switch transmission\n",
                       30);
}
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan) {
  HAL_UART_Transmit_IT(uart, (const uint8_t *)"completed switch transmission\n",
                       30);
}
