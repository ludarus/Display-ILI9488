// THIS FILE HAS BEEN DEPRECATED; NO LONGER IN USE

/*
 * commands-usart.h
 *
 *  Created on: Jun 18, 2026
 *      Author: Luke Fadel
 */

#ifndef INC_COMMANDS_USART_H_
#define INC_COMMANDS_USART_H_

#include "image.h"
#include "main.h"
#include "stm32f0xx_hal_def.h"

// public functions
HAL_StatusTypeDef UART_CMDS_Init(UART_HandleTypeDef *uartInterface,
                       SPI_HandleTypeDef *spiInterface);

// byte array struct to easily encode the size of a string
typedef struct {
  uint8_t *data;
  uint16_t size;
} ByteArray_t;

// command struct
typedef struct {
  // the keyword that must be inputted for the command
  uint8_t *keyword;
  // the size of the keyword in bytes
  uint16_t keyword_size;
  // a function pointer to a handle that executes when the command is called
  const ByteArray_t (*action)(void);
} UsartCommand_t;

#endif /* INC_COMMANDS_USART_H_ */
