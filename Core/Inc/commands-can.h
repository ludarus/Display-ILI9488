/*
 * commands-can.h
 *
 *  Created on: Jun 30, 2026
 *      Author: Luke Fadel
 */

#ifndef INC_COMMANDS_CAN_H_
#define INC_COMMANDS_CAN_H_

#include "display-ili9488.h"
#include "image.h"
#include "main.h"
#include <stdbool.h>

// object types
#define TABLE_OBJ_TYPE 13
#define BACKGROUND_OBJ_TYPE 0
#define IMAGE_OBJ_TYPE 3
#define GROUPTABLE_OBJ_TYPE 4
#define TEXT_OBJ_TYPE 1
// TODO find out what type 9 is
#define UNKNOWN_OBJ_TYPE 9

HAL_StatusTypeDef
CAN_CMDS_Init(CAN_HandleTypeDef *canInterface,
              SPI_HandleTypeDef *displaySpiInterface,
              UART_HandleTypeDef *serialLoggingInterface,
              TIM_HandleTypeDef *alarmPWMTimerInterface,
              TIM_HandleTypeDef *backlightPWMTimerInterface,
              BrightnessInfo_t brightnessSettings, GPIO_TypeDef *baudInput1Port,
              uint16_t baudInput1Pin, GPIO_TypeDef *baudInput2Port,
              uint16_t baudInput2Pin, GPIO_TypeDef *baudInput3Port,
              uint16_t baudInput3Pin);
HAL_StatusTypeDef CAN_CMDS_Process(void);

typedef struct {
  CAN_RxHeaderTypeDef header;
  uint8_t data[8];
} CanRxMessage_t;

typedef struct {
  // objNum. Should be the index in the array
  uint16_t id;
  uint8_t type;
  uint16_t x;
  uint16_t y;
  uint8_t colour;
  const Image_t *img;
} Obj_t;

typedef struct {
  // what even is this? please get clarification because it's currently just
  // an array Image_t *images[];
} Grp_t;

typedef struct {
  // the command number associated with this command
  uint8_t cmdNum;
  // a function pointer to a handle that executes when the command is called
  HAL_StatusTypeDef (*handle)(CanRxMessage_t *);
  // for logging/debugging
  uint16_t numberOfTimesCalled;
} CanCommand_t;

#endif /* INC_COMMANDS_CAN_H_ */
