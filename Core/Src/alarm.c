/*
 * alarm.c
 *
 *  Created on: Jul 6, 2026
 *      Author: Luke Fadel
 */

#include "alarm.h"
#include <stdbool.h>

// frequencies for the alarm in Hz
#define CLOCK0 14400
#define CLOCK1 7200
#define CLOCK2 4800
#define CLOCK3 3600
#define CLOCK4 2880
#define CLOCK5 2400
#define CLOCK6 2052
#define CLOCK7 28800

// list of pwm settings that create the desired frequencies above
static const PwmSetting_t frequencySettings[8] = {
    // psc = 0 for highest granularity
    {0, 3332},  {0, 6666},  {0, 9999},  {0, 13332},
    {0, 16666}, {0, 19999}, {0, 23391}, {0, 1666}};

// shard state between functions to track alarm status
static volatile PwmSetting_t alarmState;
static volatile bool alarmEnabled = false;

// function to set the alarm's state and frequency.
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

  // updating state
  alarmState.psc = frequencySettings[frequencyIndex].psc;
  alarmState.arr = frequencySettings[frequencyIndex].arr;

  // changing prescaler, autoreload & pulse settings for the desired frequency
  // using inputted duty cycle. TODO confirm to use inputted duty or set it to
  // 50%
  __HAL_TIM_SET_PRESCALER(alarmTimer, frequencySettings[frequencyIndex].psc);
  __HAL_TIM_SET_AUTORELOAD(alarmTimer, frequencySettings[frequencyIndex].arr);
  __HAL_TIM_SET_COMPARE(
      alarmTimer, TIM_CHANNEL_1,
      (uint16_t)((frequencySettings[frequencyIndex].arr * dutyCycle) / 0xFF));
}

// function to enable the alarm.
// note: this function is unused in the final code
void ALARM_Enable(TIM_HandleTypeDef *alarmTimer) {
  alarmEnabled = true;
  // setting duty cycle to 50%
  __HAL_TIM_SET_COMPARE(alarmTimer, TIM_CHANNEL_1,
                        (uint16_t)((alarmState.arr) / 2));
}

// function to disable the alarm. used in main init sequence
void ALARM_Disable(TIM_HandleTypeDef *alarmTimer) {
  alarmEnabled = false;
  // setting duty cycle to 0
  __HAL_TIM_SET_COMPARE(alarmTimer, TIM_CHANNEL_1, 0);
}

// function to start a beep.
// a beep will only sound if the alarm is not playing
void ALARM_StartBeep(TIM_HandleTypeDef *alarmTimer) {
  // Beep acts seperately from alarm, so an alarm should override a beep
  if (!alarmEnabled) {
    // using frequency from index 4 and 50% duty cycle
    __HAL_TIM_SET_PRESCALER(alarmTimer, frequencySettings[4].psc);
    __HAL_TIM_SET_AUTORELOAD(alarmTimer, frequencySettings[4].arr);
    __HAL_TIM_SET_COMPARE(alarmTimer, TIM_CHANNEL_1,
                          (uint16_t)(frequencySettings[4].arr / 2));
  }
}

// function to stop a beep
void ALARM_StopBeep(TIM_HandleTypeDef *alarmTimer) {
  if (!alarmEnabled) {
    // setting duty cycle to 0%
    __HAL_TIM_SET_COMPARE(alarmTimer, TIM_CHANNEL_1, 0);
  }
}
