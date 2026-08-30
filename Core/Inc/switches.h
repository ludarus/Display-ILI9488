/*
 * switches.h
 *
 *  Created on: Jul 6, 2026
 *      Author: Luke Fadel
 */

#include "stm32f0xx_hal.h"

#ifndef INC_SWITCHES_H_
#define INC_SWITCHES_H_
  
HAL_StatusTypeDef SWITCHES_Process(CAN_HandleTypeDef *canInterface);

#endif /* INC_SWITCHES_H_ */
