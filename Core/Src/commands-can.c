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
#include "display-ili9488-colour.h"
#include "font.h"
#include "main.h"
#include "stm32f091xc.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_can.h"
#include "stm32f0xx_hal_def.h"
#include "stm32f0xx_hal_flash.h"
#include "stm32f0xx_hal_flash_ex.h"
#include "stm32f0xx_hal_gpio.h"
#include "stm32f0xx_hal_spi.h"
#include "stm32f0xx_hal_tim.h"
#include "stm32f0xx_hal_uart.h"
#include "tables.h"
#include <stdbool.h>
#include <stdint.h>
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

// tracking the number of messages in the queue and the amount of messages being
// processed in the main loop
static volatile uint8_t writeIdx = 0; // next slot the interrupt will write to
static volatile uint8_t readIdx = 0;  // next slot the main loop will read
// CAN message queue
#define QUEUE_SIZE 64
static CanRxMessage_t queue[QUEUE_SIZE];
// static brightness members (shared state)
#define BRIGHTNESS_TABLE_SIZE 40
static uint32_t brightnessTick;
static uint8_t brightnessIdx;
static uint8_t prevBrightnessIdx;
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

      prevBrightnessIdx = *(__IO uint8_t *)(offset + BRIGHTNESS_PAGE_ADDR);

      return ILI9488_SetBrightness(spi, backlightTimer, prevBrightnessIdx);
    }
  }

  // default value if one can't be found in flash
  flashOffset = 0;
  HAL_UART_Transmit_IT(uart, (uint8_t *)"could not find previous flash value\n",
                       36);

  return ILI9488_SetBrightness(spi, backlightTimer, 29);
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
  if (objNum > 149 || objNum == 0) {
    return HAL_ERROR;
  }

  const Obj_t *obj = &objects[objNum - 1];
  Image_t *bg = (Image_t *)obj->img;

  // if it's not a background type or it has no image, SET background to blank
  if (obj->type != BACKGROUND_OBJ_TYPE || obj->img == NULL) {
    bg = (Image_t *)&File_005_ObjNum_004_480x320_6_18_26;
  }

  HAL_SPIN(ILI9488_SetBackground(spi, bg));

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

  // conditional flag that starts a new text message
  if (msg->data[0] == 0) {
    // obj number
    uint8_t lsb = msg->data[2];
    uint16_t msb = msg->data[3] << 8;

    objNum = lsb | msb;

    // number of characters in the string
    remainingChars = msg->data[1];
    target = remainingChars;

    // object num is out of range or if object isn't a text type
    if (objNum > 149 || objNum == 0 ||
        objects[objNum - 1].type != TEXT_OBJ_TYPE) {
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
    HAL_SPIN(ILI9488_BlitText(spi, objects[objNum - 1].x, objects[objNum - 1].y,
                              charArray, target, objects[objNum - 1].colour,
                              true));

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
  if (objNum > 149 || objNum == 0) {
    return HAL_ERROR;
  }

  const Obj_t *obj = &objects[objNum - 1];

  // if the obj is not an image type or has no associated image
  if (obj->type != IMAGE_OBJ_TYPE || obj->img == NULL) {
    return HAL_ERROR;
  }

  // display according image
  HAL_SPIN(
      ILI9488_BlitImage(spi, obj->x, obj->y, obj->img, obj->colour, false));

  // diagnostic logging
  // uint8_t len = snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg),
  //                        "displayed image with objNum: %u\n", objNum);

  // HAL_UART_Transmit_IT(uart, diagnosticMsg, len);

  return HAL_OK;
}

HAL_StatusTypeDef CMD_DispGrp(CanRxMessage_t *msg) {
  // This command likely isn't used within the project

  // cmdNum = 0x86
  // data format = LSB_OBJ_NUM, MSB_OBJ_NUM
  // assuming this means DLC = 2 bytes

  // extracting LSB byte
  // uint8_t lsb = msg->data[0];

  // extracting MSB byte
  // uint16_t msb = msg->data[1] << 8;

  // uint16_t grpNum = lsb | msb;

  // uint8_t index = msg->data[2];

  // if the objNum is out of range
  // if (grpNum > 149 || grpNum == 0) {
  // return HAL_ERROR;
  // }

  // const Obj_t *obj = &objects[grpNum - 1];

  // if the object isn't a group table type
  // if (obj->type != GROUPTABLE_OBJ_TYPE) {
  // return HAL_ERROR;
  // }

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

  versionHeader.TransmitGlobalTime = DISABLE;

  uint32_t mailbox;

  // HAL_UART_Transmit_IT(uart, (uint8_t *)"sending version\n", 16);

  return HAL_CAN_AddTxMessage(can, &versionHeader, version, &mailbox);
}

HAL_StatusTypeDef CMD_SysFail(CanRxMessage_t *msg) {
  // cmdNum 0x88
  HAL_UART_Transmit_IT(uart, (uint8_t *)"ERROR: SYSTEM FAILURE RECEIVED \n",
                       32);

  ILI9488_BlitImage(spi, 0, 0, &SYSFAIL_480x320, COLOR_RED, true);
  return HAL_OK;
}

HAL_StatusTypeDef CMD_Brightness(CanRxMessage_t *msg) {
  // cmdNum 0x89
  uint8_t brightnessFlag = msg->data[0];
  uint32_t thisTick = HAL_GetTick();

  switch (brightnessFlag) {
  case 1:
    // decrement brightness
    if (brightnessIdx == 0) {
      // start beep
      ALARM_StartBeep(alarmTimer);
      alarmTick = thisTick;
    } else {
      brightnessIdx--;
    }
    break;
  case 2:
    // increment brightness
    if (brightnessIdx == BRIGHTNESS_TABLE_SIZE - 1) {
      // start beep
      ALARM_StartBeep(alarmTimer);
      alarmTick = thisTick;
    } else {
      brightnessIdx++;
    }
    break;
  case 3:
    // reset brightness
    brightnessIdx = 29;
    break;
  }

  // setting brightness
  HAL_TRY(ILI9488_SetBrightness(spi, backlightTimer, brightnessIdx));

  // setting flag to wait 5 seconds
  brightnessTick = thisTick;

  // diagnostic logging
  uint8_t len = snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg),
                         "brightIdx = %u\n", brightnessIdx);

  HAL_UART_Transmit_IT(uart, diagnosticMsg, len);

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

HAL_StatusTypeDef
CAN_CMDS_Init(CAN_HandleTypeDef *canInterface,
              SPI_HandleTypeDef *displaySpiInterface,
              UART_HandleTypeDef *serialLoggingInterface,
              TIM_HandleTypeDef *alarmPWMTimerInterface,
              TIM_HandleTypeDef *backlightPWMTimerInterface,
              GPIO_TypeDef *baudInput1Port, uint16_t baudInput1Pin,
              GPIO_TypeDef *baudInput2Port, uint16_t baudInput2Pin,
              GPIO_TypeDef *baudInput3Port, uint16_t baudInput3Pin) {

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

  // setting CAN baud rate based off of input pins
  // reminder: base clock frequency = 48MHz
  canInterface->Init.Mode = CAN_MODE_NORMAL;
  canInterface->Init.TimeTriggeredMode = DISABLE;
  canInterface->Init.AutoBusOff = DISABLE;
  canInterface->Init.AutoWakeUp = DISABLE;
  canInterface->Init.AutoRetransmission = ENABLE;
  canInterface->Init.ReceiveFifoLocked = DISABLE;
  canInterface->Init.TransmitFifoPriority = DISABLE;

  if (HAL_GPIO_ReadPin(baudInput3Port, baudInput3Pin) == GPIO_PIN_RESET) {
    // 670kb baud (more accurately 666.666 baud)
    canInterface->Init.Prescaler = 4;
    canInterface->Init.TimeSeg1 = CAN_BS1_15TQ;
    canInterface->Init.TimeSeg2 = CAN_BS2_2TQ;
    canInterface->Init.SyncJumpWidth = CAN_SJW_1TQ;

    // logging message
    HAL_SPIN(HAL_UART_Transmit_IT(uart, (uint8_t *)"Bitrate = 670kbps\n", 18));

  } else if (HAL_GPIO_ReadPin(baudInput2Port, baudInput2Pin) ==
             GPIO_PIN_RESET) {
    // 500kb baud
    canInterface->Init.Prescaler = 6;
    canInterface->Init.TimeSeg1 = CAN_BS1_13TQ;
    canInterface->Init.TimeSeg2 = CAN_BS2_2TQ;
    canInterface->Init.SyncJumpWidth = CAN_SJW_1TQ;

    // logging message
    HAL_SPIN(HAL_UART_Transmit_IT(uart, (uint8_t *)"Bitrate = 500kbps\n", 18));

  } else if (HAL_GPIO_ReadPin(baudInput1Port, baudInput1Pin) ==
             GPIO_PIN_RESET) {
    // 250kb baud
    canInterface->Init.Prescaler = 12;
    canInterface->Init.TimeSeg1 = CAN_BS1_13TQ;
    canInterface->Init.TimeSeg2 = CAN_BS2_2TQ;
    canInterface->Init.SyncJumpWidth = CAN_SJW_1TQ;

    // logging message
    HAL_SPIN(HAL_UART_Transmit_IT(uart, (uint8_t *)"Bitrate = 250kbps\n", 18));

  } else {
    // 125kb baud
    canInterface->Init.Prescaler = 24;
    canInterface->Init.TimeSeg1 = CAN_BS1_13TQ;
    canInterface->Init.TimeSeg2 = CAN_BS2_2TQ;
    canInterface->Init.SyncJumpWidth = CAN_SJW_1TQ;

    // logging message
    HAL_SPIN(HAL_UART_Transmit_IT(uart, (uint8_t *)"Bitrate = 125kbps\n", 18));
  }

  // initializing can
  HAL_TRY(HAL_CAN_Init(canInterface));
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

    HAL_SPIN(ILI9488_BlitImage(spi, 0, 0, &SYSFAIL_480x320, COLOR_RED, true));

    lastMsgTick = 0;
  }

  // if the alarm has been on for 100 ms
  if (currentTick - alarmTick >= 100 && alarmTick != 0) {

    // HAL_UART_Transmit_IT(uart, (uint8_t *)"Brightness beep completed\n",
    // 26);

    ALARM_StopBeep(alarmTimer);

    alarmTick = 0;
  }

  // if it's been 5 seconds since last brightness change
  if (brightnessIdx != prevBrightnessIdx && brightnessTick != 0 &&
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
    uint16_t halfword = (uint16_t)brightnessIdx | 0x0000u;

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

    prevBrightnessIdx = brightnessIdx;

    // diagnostic logging
    HAL_UART_Transmit_IT(
        uart, (uint8_t *)"Successfully wrote brightness to flash\n", 39);
  }

  // logging
  // if (readIdx != writeIdx) {
  //   // logging
  //   uint8_t len = snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg),
  //                          "%u, %u\n", readIdx, writeIdx);
  //
  //   HAL_UART_Transmit_IT(uart, diagnosticMsg, len);
  // }

  // iterating through every message
  while (readIdx != writeIdx) {

    CanRxMessage_t *msg = &queue[readIdx];

    uint8_t cmdNum = (uint8_t)(msg->header.StdId >> 3);

    // assuming the commandnums are contiguous and in the correct order
    if (cmdNum >= commands[0].cmdNum &&
        cmdNum <=
            commands[0].cmdNum + (sizeof(commands) / sizeof(commands[0])) - 1) {

      // executing command
      commands[cmdNum - commands[0].cmdNum].handle(msg);

      // incrementing call log
      // commands[cmdNum - commands[0].cmdNum].numberOfTimesCalled++;

    } else {
      // if no commands match
      readIdx = (readIdx + 1) & (QUEUE_SIZE - 1);
      return HAL_ERROR;
    }

    readIdx = (readIdx + 1) & (QUEUE_SIZE - 1);
  }

  return HAL_OK;
}

// callback for received message
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  // setting last tick
  lastMsgTick = HAL_GetTick();

  // nextWriteIdx = (writeIdx + 1) % QUEUE_SIZE
  // The & optimization only works of QUEUE_SIZE is a power of 2
  uint8_t nextWriteIdx = (writeIdx + 1) & (QUEUE_SIZE - 1);

  // when the write idx has almost "lapped" the read index
  if (nextWriteIdx == readIdx) {
    // on queue overflow - write message to junk buffer to discard it
    CAN_RxHeaderTypeDef hdr;
    uint8_t data[8];
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &hdr, data);
    overFlowed = true;
  } else {
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &queue[writeIdx].header,
                         queue[writeIdx].data);
  }

  // incrementing writeidx
  writeIdx = nextWriteIdx;
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
