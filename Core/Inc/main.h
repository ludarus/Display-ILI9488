/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

// macro that returns thrown error code if executed function isn't HAL_OK
#define HAL_TRY(x)                                                             \
  do {                                                                         \
    HAL_StatusTypeDef _s = (x);                                                \
    if (_s != HAL_OK)                                                          \
      return _s;                                                               \
  } while (0)

// spinning macro to wait until a function isn't busy and execute it
#define HAL_SPIN(x)                                                            \
  while (x == HAL_BUSY) {                                                      \
  }
/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define ALARM_Pin GPIO_PIN_4
#define ALARM_GPIO_Port GPIOA
#define DISPLAY_DC_Pin GPIO_PIN_4
#define DISPLAY_DC_GPIO_Port GPIOC
#define BAUD_INPUT3_Pin GPIO_PIN_5
#define BAUD_INPUT3_GPIO_Port GPIOC
#define BAUD_INPUT2_Pin GPIO_PIN_6
#define BAUD_INPUT2_GPIO_Port GPIOC
#define BAUD_INPUT1_Pin GPIO_PIN_8
#define BAUD_INPUT1_GPIO_Port GPIOC
#define SWITCH_POWER_Pin GPIO_PIN_10
#define SWITCH_POWER_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define DISPLAY_CS_Pin GPIO_PIN_15
#define DISPLAY_CS_GPIO_Port GPIOA
#define SWITCH5_Pin GPIO_PIN_11
#define SWITCH5_GPIO_Port GPIOC
#define SWITCH4_Pin GPIO_PIN_12
#define SWITCH4_GPIO_Port GPIOC
#define SWITCH3_Pin GPIO_PIN_2
#define SWITCH3_GPIO_Port GPIOD
#define SWITCH2_Pin GPIO_PIN_3
#define SWITCH2_GPIO_Port GPIOB
#define SWITCH1_Pin GPIO_PIN_4
#define SWITCH1_GPIO_Port GPIOB
#define DISPLAY_RESET_Pin GPIO_PIN_6
#define DISPLAY_RESET_GPIO_Port GPIOB
#define DISPLAY_LED_Pin GPIO_PIN_9
#define DISPLAY_LED_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
