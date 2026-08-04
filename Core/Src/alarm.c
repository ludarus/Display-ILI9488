/*
 * alarm.c
 *
 *  Created on: Jul 6, 2026
 *      Author: Luke Fadel
 */

#include "alarm.h"
#include "stm32f0xx_hal_tim.h"
#include <stdbool.h>
#include <stdint.h>

static const PwmSetting_t frequencySettings[8] = {
    // psc = 0 for highest granularity
    {0, 3332},  {0, 6666},  {0, 9999},  {0, 13332},
    {0, 16666}, {0, 19999}, {0, 23391}, {0, 1666}};

// shard state
static volatile PwmSetting_t alarmState;
static volatile bool alarmEnabled = false;

// pass in a value from 0-7 for frequency, and a value from 0-255 for duty cycle
void ALARM_Set(TIM_HandleTypeDef *alarmTimer, uint8_t frequencyIndex,
               uint8_t dutyCycle) {

  if (frequencyIndex > 7) {
    return;
  }

  // setting alarm state
  if (dutyCycle != 0) {
    alarmEnabled = true;
  } else {
    alarmEnabled = false;
  }

  alarmState.psc = frequencySettings[frequencyIndex].psc;
  alarmState.arr = frequencySettings[frequencyIndex].arr;

  // changing the prescaler, autoreload, and pulse settings for the desired
  // frequency
  __HAL_TIM_SET_PRESCALER(alarmTimer, frequencySettings[frequencyIndex].psc);
  __HAL_TIM_SET_AUTORELOAD(alarmTimer, frequencySettings[frequencyIndex].arr);
  __HAL_TIM_SET_COMPARE(
      alarmTimer, TIM_CHANNEL_1,
      (uint16_t)((frequencySettings[frequencyIndex].arr * dutyCycle) / 0xFF));
}

// using 50% duty cycle as on. TODO confirm this
void ALARM_Enable(TIM_HandleTypeDef *alarmTimer) {
  alarmEnabled = true;
  __HAL_TIM_SET_COMPARE(alarmTimer, TIM_CHANNEL_1,
                        (uint16_t)((alarmState.arr) / 2));
}

void ALARM_Disable(TIM_HandleTypeDef *alarmTimer) {
  alarmEnabled = false;
  __HAL_TIM_SET_COMPARE(alarmTimer, TIM_CHANNEL_1, 0);
}

void ALARM_StartBeep(TIM_HandleTypeDef *alarmTimer) {
  // Beep acts seperately from alarm, so an alarm should override a beep
  if (!alarmEnabled) {
    __HAL_TIM_SET_PRESCALER(alarmTimer, frequencySettings[4].psc);
    __HAL_TIM_SET_AUTORELOAD(alarmTimer, frequencySettings[4].arr);
    __HAL_TIM_SET_COMPARE(alarmTimer, TIM_CHANNEL_1,
                          (uint16_t)(frequencySettings[4].arr / 2));
  }
}

void ALARM_StopBeep(TIM_HandleTypeDef *alarmTimer) {
  if (!alarmEnabled) {
    __HAL_TIM_SET_COMPARE(alarmTimer, TIM_CHANNEL_1, 0);
  }
}
