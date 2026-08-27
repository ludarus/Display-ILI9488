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
#include "stm32f0xx_hal.h"

// CAN message queue
#define QUEUE_SIZE 64
// brightness table size
#define BRIGHTNESS_TABLE_SIZE 40

// object types
#define TABLE_OBJ_TYPE 13
#define BACKGROUND_OBJ_TYPE 0
#define IMAGE_OBJ_TYPE 3
#define GROUPTABLE_OBJ_TYPE 4
#define TEXT_OBJ_TYPE 1
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

// struct to store basic info for a received CAN message
typedef struct {
  CAN_RxHeaderTypeDef header;
  uint8_t data[8];
} CanRxMessage_t;

// struct that stores object information
typedef struct {
  // objNum. Should be the index in the array
  uint16_t id;
  // object type. Equal to one of the enums at the top of commands-can.h
  uint8_t type;
  // x position of object
  uint16_t x;
  // y position of object
  uint16_t y;
  // Equal to one of the enums at the top of display-ili9488.h
  uint8_t colour;
  // pointer to the image if applicable
  const Image_t *img;
} Obj_t;

// struct that stores information for CAN command handles
typedef struct {
  // the command number associated with this command
  const uint8_t cmdNum;
  // a function pointer to a handle that executes when the command is called
  HAL_StatusTypeDef (*handle)(CanRxMessage_t *);
} CanCommand_t;

#endif /* INC_COMMANDS_CAN_H_ */
