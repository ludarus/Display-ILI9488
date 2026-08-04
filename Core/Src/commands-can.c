/*
 * commands-can.c
 *
 *  Created on: Jun 30, 2026
 *      Author: Luke Fadel
 */

#include "commands-can.h"
#include "File_005_ObjNum_004_480x320_6_18_26.h"
#include "SYSFAIL_480x320.h"
#include "alarm.h"
#include "display-ili9488.h"
#include "font.h"
#include "main.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_can.h"
#include "stm32f0xx_hal_def.h"
#include "stm32f0xx_hal_flash.h"
#include "stm32f0xx_hal_flash_ex.h"
#include "stm32f0xx_hal_spi.h"
#include "stm32f0xx_hal_tim.h"
#include "stm32f0xx_hal_uart.h"
#include "tables.h"
#include <stdbool.h>
#include <stdio.h>

// failure image
#include "SYSFAIL_480x320.h"

// TODO get correct version string
const uint8_t version[8] = "DSP12345";

// debugging ---
// overflow flag
volatile static bool overFlowed = false;
// debugging ---

// private declarations

// number of queued messages to be processed
static volatile uint8_t queuedMessages = 0;
// CAN message queue
static CanRxMessage_t queue[48];
// static brightness members (shared state)
static uint32_t brightnessTick;
static uint8_t brightnessVal;
static uint8_t prevBrightnessVal;
static uint16_t flashOffset;

// alarm members
static volatile uint32_t alarmTick;

// for error
static volatile uint32_t lastMsgTick = 1;

// for serial diagnostics
static uint8_t diagnosticMsg[64];

// interfaces
static CAN_HandleTypeDef *can;
static SPI_HandleTypeDef *spi;
static UART_HandleTypeDef *uart;
static TIM_HandleTypeDef *alarmTimer;
static TIM_HandleTypeDef *backlightTimer;

// private functions

// TODO move this?
HAL_StatusTypeDef brightnessInit() {
  // reading flash to get last value of pointer
  // two bytes per half word
  for (int32_t offset = FLASH_PAGE_SIZE - 2; offset >= 0; offset -= 2) {
    // checking if the 16 bit half word is smaller than the default value
    // protocol: store the brightness val in the first 8 bits of the halfword,
    // then set the last 8 bits to 0 to indicate that the byte has been written
    if (*(__IO uint16_t *)(offset + BRIGHTNESS_PAGE_ADDR) < 0xFFFF) {
      flashOffset = (uint32_t)offset + 2;
      // setting brightness
      // diagnostic logging
      uint8_t len =
          snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg),
                   "successfully read previous brightness value, offset = %u\n",
                   flashOffset);

      HAL_UART_Transmit_IT(uart, diagnosticMsg, len);

      prevBrightnessVal = *(__IO uint8_t *)(offset + BRIGHTNESS_PAGE_ADDR);

      return ILI9488_SetBrightness(spi, backlightTimer, prevBrightnessVal);
    }
  }

  // default value if one can't be found in flash
  flashOffset = 0;
  HAL_UART_Transmit_IT(uart, (uint8_t *)"could not find previous flash value\n",
                       36);

  return ILI9488_SetBrightness(spi, backlightTimer, 0xFF);
}

// handles. TODO finish them when given the objnum and groupnum to image mapping
HAL_StatusTypeDef CMD_DispBg(CanRxMessage_t *msg) {
  // cmdNum = 0x83
  // id = 0x418
  // data format = LSB_OBJ_NUM, MSB_OBJ_NUM
  // assuming this means DLC = 2 bytes

  // extracting LSB byte
  uint8_t lsb = msg->data[0];

  // extracting MSB byte
  uint16_t msb = msg->data[1] << 8;

  uint16_t objNum = lsb | msb;

  // objnum checking
  if (objNum > 149) {
    return HAL_ERROR;
  }

  const Obj_t *obj = &objects[objNum - 1];
  Image_t *bg = (Image_t *)obj->img;

  // if it's not a background type or it has no image, SET background to blank
  if (obj->type != BACKGROUND_OBJ_TYPE || obj->img == NULL) {
    bg = (Image_t *)&File_005_ObjNum_004_480x320_6_18_26;
  }

  HAL_TRY(ILI9488_SetBackground(bg));

  HAL_SPIN(ILI9488_LoadImage(spi, 0, 0, bg, true, false, true));

  // display according image logging
  // uint8_t len = snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg),
  //                        "displayed background with objNum: %u\n", objNum);

  // HAL_UART_Transmit_IT(uart, diagnosticMsg, len);

  return HAL_OK;
}

HAL_StatusTypeDef CMD_DispText(CanRxMessage_t *msg) {
  // cmdNum = 0x84
  // id = 0x420

  // static function members for persistent scope
  static uint8_t remainingChars = 0;
  static uint8_t target = 0;
  static uint8_t charArray[256] = {0};
  static uint16_t objNum = 0;

  // checking if DLC is 0
  if (msg->header.DLC == 0) {
    // ignore command
    return HAL_OK;
  }

  if (msg->data[0] == 0) {
    // should have a value of 0
    // uint8_t First_Pkt_Flag = msg->data[0];

    // number of characters in the string
    remainingChars = msg->data[1];
    target = remainingChars;
    // obj number
    uint8_t lsb = msg->data[2];
    uint16_t msb = msg->data[3] << 8;

    objNum = lsb | msb;

    // if object isn't a text type
    if (objects[objNum - 1].type != TEXT_OBJ_TYPE) {
      // resetting target
      target = 0;
      return HAL_ERROR;
    }

    // filling up the remaining bytes of charInfo contained within this packet
    uint8_t fill = (remainingChars > 4) ? 4 : remainingChars;
    for (uint8_t i = 0; i < fill; i++) {
      charArray[i] = msg->data[i + 4];
      remainingChars--;
    }
  }
  // continuation of previous packet
  else if (target != 0) {
    uint8_t fill = (remainingChars > 8) ? 8 : remainingChars;
    uint8_t offset = target - remainingChars;
    for (uint8_t i = 0; i < fill; i++) {
      charArray[i + offset] = msg->data[i];
      remainingChars--;
    }
  }
  // end condition
  if (remainingChars == 0 && target != 0) {
    // displaying
    // HAL_StatusTypeDef displayStatus =
    HAL_SPIN(ILI9488_LoadText(spi, objects[objNum - 1].x, objects[objNum - 1].y,
                              charArray, target, font, FONTSIZE, CHARWIDTH,
                              CHARHEIGHT, false, true, true));

    // uint8_t len = snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg),
    //                        "Disp text: \"%.*s\", objNum = %u\n", target,
    //                        charArray, objNum);

    // HAL_UART_Transmit_IT(uart, diagnosticMsg, len);

    // restting target
    target = 0;
    // return displayStatus;
    return HAL_OK;
  }

  // logging
  // HAL_UART_Transmit_IT(uart, (uint8_t *)"recieved partial text packet\n",
  // 29);

  return HAL_OK;
}

HAL_StatusTypeDef CMD_DispImage(CanRxMessage_t *msg) {
  // cmdNum = 0x85

  // data format = LSB_OBJ_NUM, MSB_OBJ_NUM
  // assuming this means DLC = 2 bytes

  // extracting LSB byte
  uint8_t lsb = msg->data[0];

  // extracting MSB byte
  uint16_t msb = msg->data[1] << 8;

  uint16_t objNum = lsb | msb;

  // objnum checking
  if (objNum > 149) {
    return HAL_ERROR;
  }

  const Obj_t *obj = &objects[objNum - 1];

  // if the obj is not an image type or has no associated image
  if (obj->type != IMAGE_OBJ_TYPE || obj->img == NULL) {
    return HAL_ERROR;
  }

  // display according image
  HAL_SPIN(ILI9488_LoadImage(spi, obj->x, obj->y, obj->img, false, true, true));

  // diagnostic logging
  // uint8_t len = snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg),
  //                        "displayed image with objNum: %u\n", objNum);

  // HAL_UART_Transmit_IT(uart, diagnosticMsg, len);

  return HAL_OK;
}

HAL_StatusTypeDef CMD_DispGrp(CanRxMessage_t *msg) {

  // cmdNum = 0x86
  // data format = LSB_OBJ_NUM, MSB_OBJ_NUM
  // assuming this means DLC = 2 bytes

  // extracting LSB byte
  uint8_t lsb = msg->data[0];

  // extracting MSB byte
  uint16_t msb = msg->data[1] << 8;

  uint16_t grpNum = lsb | msb;

  uint8_t index = msg->data[2];

  const Obj_t *obj = &objects[grpNum - 1];

  // if the object isn't a group table type
  if (obj->type != GROUPTABLE_OBJ_TYPE) {
    return HAL_ERROR;
  }

  // display according image
  // ILI9488_LOAD_IMAGE(spi, uint16_t x, uint16_t y, const Image_t *image,
  // bool overWrite, bool draw)

  // diagnostic logging
  // uint8_t len = snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg),
  //                        "displayed group with grpNum: %u and index: %u\n",
  //                        grpNum, index);

  // HAL_UART_Transmit_IT(uart, diagnosticMsg, len);

  return HAL_OK;
}

HAL_StatusTypeDef CMD_SendVersion(CanRxMessage_t *msg) {
  // cmdNum 0x87

  CAN_TxHeaderTypeDef versionHeader = {0};

  // 8 byte string
  versionHeader.DLC = 8;

  // as specified in protocol
  versionHeader.StdId = 0x168;

  versionHeader.IDE = CAN_ID_STD;
  versionHeader.RTR = CAN_RTR_DATA;

  // TODO: confirm this
  versionHeader.TransmitGlobalTime = DISABLE;

  uint32_t mailbox;

  // HAL_UART_Transmit_IT(uart, (uint8_t *)"sending version\n", 16);

  return HAL_CAN_AddTxMessage(can, &versionHeader, version, &mailbox);
}

HAL_StatusTypeDef CMD_SysFail(CanRxMessage_t *msg) {
  // cmdNum 0x88
  HAL_UART_Transmit_IT(uart, (uint8_t *)"ERROR: SYSTEM FAILURE RECEIVED \n",
                       32);

  ILI9488_LoadImage(spi, 0, 0, &SYSFAIL_480x320, true, false, true);
  return HAL_OK;
  // return ILI9488_LoadImage(spi, 0, 0, &SYSFAIL_480x320, true, false, true);
}

HAL_StatusTypeDef CMD_Brightness(CanRxMessage_t *msg) {
  // cmdNum 0x89
  brightnessVal = msg->data[0];

  // setting brightness
  HAL_TRY(ILI9488_SetBrightness(spi, backlightTimer, brightnessVal));

  // setting flag to wait 5 seconds
  brightnessTick = HAL_GetTick();

  // if max value has been reached TODO: check if this should only happen after
  // the max value is already set
  if (brightnessVal == 0xFF || brightnessVal == 0) {
    ALARM_StartBeep(alarmTimer);
    alarmTick = brightnessTick;
  }

  // diagnostic logging
  // uint8_t len = snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg),
  //                        "changed display brightness to %u\n",
  //                        brightnessVal);

  // HAL_UART_Transmit_IT(uart, diagnosticMsg, len);

  return HAL_OK;
}

HAL_StatusTypeDef CMD_Alarm(CanRxMessage_t *msg) {
  // assuming dutyCycle is an 8 bit number where 0 is off and 255 is 100%
  uint8_t dutyCycle = msg->data[0];
  uint8_t frequency = msg->data[1];

  ALARM_Set(alarmTimer, frequency, dutyCycle);

  // logging
  // uint8_t len =
  //     snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg),
  //              "changed alarm to a frequency of %u and a duty of %u\n",
  //              frequency, dutyCycle);

  // HAL_UART_Transmit_IT(uart, diagnosticMsg, len);

  return HAL_OK;
}

// list of commands to be received
// const uncommented for debugging logs
static /*const*/ CanCommand_t commands[] = {

    // display background image
    // 0x418
    {.cmdNum = 0x83, .handle = CMD_DispBg},

    // display text
    // 0x420
    {.cmdNum = 0x84, .handle = CMD_DispText},

    // display image
    // 0x428
    {.cmdNum = 0x85, .handle = CMD_DispImage},

    // display group
    // 0x430
    {.cmdNum = 0x86, .handle = CMD_DispGrp},

    // send version
    // 0x438
    {.cmdNum = 0x87, .handle = CMD_SendVersion},

    // system failure
    // 0x440
    {.cmdNum = 0x88, .handle = CMD_SysFail},

    // adjust brightness
    // 0x448
    {.cmdNum = 0x89, .handle = CMD_Brightness},

    // alarm
    // 0x450
    {.cmdNum = 0x8A, .handle = CMD_Alarm},
};

// public functions

HAL_StatusTypeDef CAN_CMDS_Init(CAN_HandleTypeDef *canInterface,
                                SPI_HandleTypeDef *displaySpiInterface,
                                UART_HandleTypeDef *serialLoggingInterface,
                                TIM_HandleTypeDef *alarmPWMTimerInterface,
                                TIM_HandleTypeDef *backlightPWMTimerInterface) {

  can = canInterface;
  spi = displaySpiInterface;
  uart = serialLoggingInterface;
  alarmTimer = alarmPWMTimerInterface;
  backlightTimer = backlightPWMTimerInterface;

  lastMsgTick = HAL_GetTick();

  HAL_Delay(500);
  // display brightness
  brightnessInit();

  // configuring filter to specifically only accept the specific IDS mentioned
  // in the raymond protocol
  CAN_FilterTypeDef sFilterConfig = {0};

  // note: there are 14 filter banks available

  // Bank 0: 0x418, 0x420, 0x428, 0x430
  sFilterConfig.FilterBank = 0;
  sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST;
  sFilterConfig.FilterScale = CAN_FILTERSCALE_16BIT;
  sFilterConfig.FilterIdLow = (0x418 << 5);      // 0x8300
  sFilterConfig.FilterIdHigh = (0x420 << 5);     // 0x8400
  sFilterConfig.FilterMaskIdLow = (0x428 << 5);  // 0x8500
  sFilterConfig.FilterMaskIdHigh = (0x430 << 5); // 0x8600
  sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
  sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;

  HAL_TRY(HAL_CAN_ConfigFilter(canInterface, &sFilterConfig));

  // Bank 1: 0x438, 0x440, 0x448, 0x450
  sFilterConfig.FilterBank = 1;
  sFilterConfig.FilterIdLow = (0x438 << 5);      // 0x8700
  sFilterConfig.FilterIdHigh = (0x440 << 5);     // 0x8800
  sFilterConfig.FilterMaskIdLow = (0x448 << 5);  // 0x8900
  sFilterConfig.FilterMaskIdHigh = (0x450 << 5); // 0x8A00

  // Mode/scale/FIFO/activation carried over from above
  HAL_TRY(HAL_CAN_ConfigFilter(canInterface, &sFilterConfig));

  // adding a FIFO1 filter to detect idling CAN network (heartbeat only)
  // Bank 2 - 0x416, repeated to fill all 4 slots
  sFilterConfig.FilterBank = 2; // next free bank
  sFilterConfig.FilterMode = CAN_FILTERMODE_IDLIST;
  sFilterConfig.FilterScale = CAN_FILTERSCALE_16BIT;
  sFilterConfig.FilterIdLow = (0x416 << 5);
  sFilterConfig.FilterIdHigh = (0x416 << 5);
  sFilterConfig.FilterMaskIdLow = (0x416 << 5);
  sFilterConfig.FilterMaskIdHigh = (0x416 << 5);
  sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO1;
  sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;
  HAL_TRY(HAL_CAN_ConfigFilter(canInterface, &sFilterConfig));

  // starting device
  HAL_TRY(HAL_CAN_Start(canInterface));

  // enable interrupts for both FIFOs
  HAL_TRY(
      HAL_CAN_ActivateNotification(canInterface, CAN_IT_RX_FIFO0_MSG_PENDING));
  HAL_TRY(
      HAL_CAN_ActivateNotification(canInterface, CAN_IT_RX_FIFO1_MSG_PENDING));

  return HAL_OK;
}

HAL_StatusTypeDef CAN_CMDS_Process(void) {

  uint32_t lastMsgTick_StateSave = lastMsgTick;
  uint32_t currentTick = HAL_GetTick();
  // display error if the can bus is silent for 4 seconds
  if (currentTick - lastMsgTick_StateSave > 4000 &&
      lastMsgTick_StateSave != 0) {

    // display error image.
    HAL_UART_Transmit_IT(
        uart, (uint8_t *)"TIMEOUT: no command received in the last 4000ms\n",
        48);

    ILI9488_LoadImage(spi, 0, 0, &SYSFAIL_480x320, true, false, true);

    lastMsgTick = 0;
  }

  // if the alarm has been on for 100 ms
  if (currentTick - alarmTick >= 100 && alarmTick != 0) {

    // HAL_UART_Transmit_IT(uart, (uint8_t *)"Brightness beep completed\n", 26);

    ALARM_StopBeep(alarmTimer);

    alarmTick = 0;
  }

  // if it's been 5 seconds since last brightness change
  if (brightnessVal != prevBrightnessVal && brightnessTick != 0 &&
      currentTick - brightnessTick > 5000) {
    brightnessTick = 0;

    // unlocking flash
    HAL_TRY(HAL_FLASH_Unlock());

    // if the last page of flash is full
    if (flashOffset >= FLASH_PAGE_SIZE) {
      // erasing flash
      FLASH_EraseInitTypeDef erase;
      uint32_t pageError;

      // erase the last page of flash memory
      erase.TypeErase = FLASH_TYPEERASE_PAGES;
      erase.PageAddress = BRIGHTNESS_PAGE_ADDR;
      erase.NbPages = 1;

      HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase, &pageError);
      if (status != HAL_OK) {
        HAL_FLASH_Lock();
        HAL_UART_Transmit_IT(uart,
                             (uint8_t *)"Failed to write brightness to flash "
                                        "(couldn't erase flash)\n",
                             59);
        return status;
      }

      flashOffset = 0;
    }

    // minimum flash write resolution is 16 bit (halfword)
    // making second part off to indicate a write
    uint16_t halfword = (uint16_t)brightnessVal | 0x0000u;

    // TODO error handling here
    HAL_StatusTypeDef progStatus =
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                          BRIGHTNESS_PAGE_ADDR + flashOffset, halfword);
    HAL_FLASH_Lock();

    if (progStatus != HAL_OK) {
      return progStatus;
    }

    // incrementing offset
    flashOffset += 2;

    prevBrightnessVal = brightnessVal;

    // diagnostic logging
    HAL_UART_Transmit_IT(
        uart, (uint8_t *)"Successfully wrote brightness to flash\n", 39);
  }

  // iterating through every message
  uint8_t snapshot = queuedMessages;
  if (snapshot != 0) {
    // logging
    uint8_t len = snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg), "%u\n",
                           snapshot);

    HAL_UART_Transmit_IT(uart, diagnosticMsg, len);
  }
  for (uint8_t msgIdx = 0; msgIdx < snapshot; msgIdx++) {

    // // Format the message
    // uint8_t len = snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg),
    //                        "received CAN msg with id: 0x%03lX\n",
    //                        (unsigned long)queue[msgIdx].header.StdId);
    //
    // // Transmit only the characters that were written
    // HAL_UART_Transmit_IT(uart, diagnosticMsg, len);

    queuedMessages--;

    uint8_t cmdNum = (uint8_t)(queue[msgIdx].header.StdId >> 3);
    // assuming the commandnums are contiguous and in the correct order
    if (cmdNum >= commands[0].cmdNum &&
        cmdNum <=
            commands[0].cmdNum + (sizeof(commands) / sizeof(commands[0])) - 1) {

      // executing command
      commands[cmdNum - commands[0].cmdNum].handle(&queue[msgIdx]);

      // incrementing call log
      // commands[cmdNum - commands[0].cmdNum].numberOfTimesCalled++;

    } else {
      // if no commands match
      // Maybe return a better error than this?
      return HAL_ERROR;
    }
  }

  return HAL_OK;
}

// callback for received message
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  // queueing the command for processing in main loop. If too many messages,
  // just overflow. Maybe change this later TODO
  lastMsgTick = HAL_GetTick();
  // no error handling in interrupt. Possibly do something to fix this
  HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &queue[queuedMessages].header,
                       queue[queuedMessages].data);
  if (queuedMessages < 48) {
    queuedMessages++;
  } else {
    overFlowed = true;
  }
}

// callback for junk messages to check if network is still active
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  // setting last tick
  lastMsgTick = HAL_GetTick();
  // temporary variable allocation to store junk message
  CAN_RxHeaderTypeDef hdr;
  uint8_t data[8];

  HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &hdr, data);
}
