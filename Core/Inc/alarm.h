/*
 * alarm.h
 *
 *  Created on: Jul 6, 2026
 *      Author: joe
 */

#ifndef INC_ALARM_H_
#define INC_ALARM_H_

#include "main.h"
#include "stm32f0xx_hal_tim_ex.h"


void ALARM_Set(TIM_HandleTypeDef *alarmTimer, uint8_t frequencyIndex,
               uint8_t dutyCycle);
void ALARM_Enable(TIM_HandleTypeDef *alarmTimer);
void ALARM_Disable(TIM_HandleTypeDef *alarmTimer);
void ALARM_StartBeep(TIM_HandleTypeDef *alarmTimer);
void ALARM_StopBeep(TIM_HandleTypeDef *alarmTimer);

typedef struct {
  uint16_t psc;
  uint16_t arr;
} PwmSetting_t;

#endif /* INC_ALARM_H_ */
