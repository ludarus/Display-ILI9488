/*
 * commands-can.h
 *
 *  Created on: Jun 30, 2026
 *      Author: Luke Fadel
 */

#ifndef INC_COMMANDS_CAN_H_
#define INC_COMMANDS_CAN_H_

#include "image.h"
#include "main.h"
#include "stm32f0xx_hal_def.h"
#include "stm32f0xx_hal_tim.h"
#include <stdbool.h>

// object types
#define TABLE_OBJ_TYPE 13
#define BACKGROUND_OBJ_TYPE 0
#define IMAGE_OBJ_TYPE 3
#define GROUPTABLE_OBJ_TYPE 4
#define TEXT_OBJ_TYPE 1
// TODO find out what type 9 is
#define UNKNOWN_OBJ_TYPE 9

HAL_StatusTypeDef CAN_CMDS_Init(CAN_HandleTypeDef *canInterface,
                                SPI_HandleTypeDef *displaySpiInterface,
                                UART_HandleTypeDef *serialLoggingInterface,
                                TIM_HandleTypeDef *alarmPWMTimerInterface,
                                TIM_HandleTypeDef *backlightPWMTimerInterface);
HAL_StatusTypeDef CAN_CMDS_Process(void);

typedef struct {
  CAN_RxHeaderTypeDef header;
  uint8_t data[8];
} CanRxMessage_t;

typedef struct {
  // objNum. Should be the index in the array, so probably remove this field
  // later
  uint16_t id;
  uint8_t type;
  uint16_t x;
  uint16_t y;
  const Image_t *img;
} Obj_t;

typedef struct {
  // what even is this? please get clarification because it's currently just an
  // array Image_t *images[];
} Grp_t;

typedef struct {
  // the command number associated with this command
  uint8_t cmdNum;
  // a function pointer to a handle that executes when the command is called
  HAL_StatusTypeDef (*handle)(CanRxMessage_t *);
  uint16_t numerOfTimesCalled;
} CanCommand_t;

#endif /* INC_COMMANDS_CAN_H_ */
