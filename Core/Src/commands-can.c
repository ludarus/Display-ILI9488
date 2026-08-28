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
#include "commands-can.h"
#include "display-ili9488.h"
#include "stm32f0xx_hal.h"
#include "tables.h"
#include <stdio.h>

//--------------------------------------------------------------------------------
// global variables

const uint8_t version[8] = "DSP12345";

// tracking the number of messages in the queue and the amount of messages being
// processed in the main loop
// circular head and tail approach
static volatile uint8_t writeIdx = 0; // next slot the interrupt will write to
static volatile uint8_t readIdx = 0;  // next slot the main loop will read

// Queued CAN messages buffer (circular buffer)
static CanRxMessage_t queue[QUEUE_SIZE];

// brightness shared state members

// tick of last brightness change
static uint32_t brightnessTick;
// index for brightnessTable
static uint8_t brightnessIdx;
// index in flash where previous brightness index was stored
static uint8_t prevBrightnessIdx;
// index in flash where brightness index is stored
static uint16_t flashOffset;

// tick to track beep state
static volatile uint32_t beepTick;

// tick to track when last CAN message was recieved
static volatile uint32_t lastMsgTick = 1;

// for serial logging and diagnostics
static uint8_t diagnosticMsg[64];

// interfaces
static CAN_HandleTypeDef *can;
static SPI_HandleTypeDef *spi;
static UART_HandleTypeDef *uart;
static TIM_HandleTypeDef *alarmTimer;
static TIM_HandleTypeDef *backlightTimer;

//--------------------------------------------------------------------------------
// private handles

// CAN display background function handle
// cmdNum = 0x83
// id = 0x418
// data format = LSB_OBJ_NUM, MSB_OBJ_NUM
// DLC = 2 bytes
HAL_StatusTypeDef CMD_DispBg(CanRxMessage_t *msg) {
  // extracting LSB byte
  uint8_t lsb = msg->data[0];

  // extracting MSB byte
  uint16_t msb = msg->data[1] << 8;

  // getting objNum
  uint16_t objNum = lsb | msb;

  // objnum checking
  if (objNum > 149 || objNum == 0) {
    return HAL_ERROR;
  }

  // assigning object and background reference for readability
  const Obj_t *obj = &objects[objNum - 1];
  Image_t *bg = (Image_t *)obj->img;

  // if it's not a background type or it has no image, SET background to blank
  if (obj->type != BACKGROUND_OBJ_TYPE || obj->img == NULL) {
    bg = (Image_t *)&File_005_ObjNum_004_480x320_6_18_26;
  }

  // setting and displaying background
  HAL_SPIN(ILI9488_SetBackground(spi, bg));

  // display according image logging
  // uint8_t len = snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg),
  //                        "displayed background with objNum: %u\n", objNum);

  // HAL_UART_Transmit_IT(uart, diagnosticMsg, len);

  return HAL_OK;
}

// CAN display text function handle
// cmdNum = 0x84
// id = 0x420
// format =
// { First_Pkt_Flag, numChars, LSB_OBJ_NUM, MSB_OBJ_NUM, Char1, Char2, Char3,
// Char4 },
// { Char5, Char6, Char7, Char8 ... CharN}
HAL_StatusTypeDef CMD_DispText(CanRxMessage_t *msg) {

  // static function members for persistent scope
  // (state saves across this function call)

  // remaining number of characters to process
  static uint8_t remainingChars = 0;
  // total number of characters in this text
  static uint8_t target = 0;
  // array to hold the characters
  // the max size should only be ILI9488_WIDTH_PX/CHARWIDTH = 15,
  // but some calls use more than that
  static uint8_t charArray[256] = {0};
  // object number
  static uint16_t objNum = 0;

  // checking if DLC is 0
  if (msg->header.DLC == 0) {
    // ignore command
    return HAL_OK;
  }

  // conditional flag that starts a new text message
  if (msg->data[0] == 0) {
    // if there was previous data from the last message,
    // overwrite it with new message

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

    // filling up the remaining bytes of the character array contained within
    // this packet
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
                              charArray, target, false

#if COLOUR_ENABLED
                              ,
                              objects[objNum - 1].colour
#endif

                              ));

    // logging
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

// CAN display image function handle
// cmdNum = 0x85
// data format = LSB_OBJ_NUM, MSB_OBJ_NUM
// DLC = 2 bytes
HAL_StatusTypeDef CMD_DispImage(CanRxMessage_t *msg) {

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
  HAL_SPIN(ILI9488_BlitImage(spi, obj->x, obj->y, obj->img, false
#if COLOUR_ENABLED
                             ,
                             obj->colour
#endif

                             ));

  // diagnostic logging
  // uint8_t len =
  //     snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg), "i %u\n",
  //     objNum);
  //
  // HAL_UART_Transmit_IT(uart, diagnosticMsg, len);

  return HAL_OK;
}

// CAN display group function handle
// cmdNum = 0x86
// id = 0x430
// data format = LSB_OBJ_NUM, MSB_OBJ_NUM
// DLC = 2 bytes
HAL_StatusTypeDef CMD_DispGrp(CanRxMessage_t *msg) {
  // This command likely isn't used within the project

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

  // diagnostic logging
  // uint8_t len = snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg),
  //                        "displayed group with grpNum: %u and index: %u\n",
  //                        grpNum, index);

  // HAL_UART_Transmit_IT(uart, diagnosticMsg, len);

  return HAL_OK;
}

// CAN send version handle
// cmdNum = 0x87
// id = 0x438
// DLC = 0 bytes
HAL_StatusTypeDef CMD_SendVersion(CanRxMessage_t *msg) {

  CAN_TxHeaderTypeDef versionHeader = {0};

  // responding with version string

  // 8 byte string
  versionHeader.DLC = 8;

  // as specified in protocol. cmdNum 0x2D
  versionHeader.StdId = 0x168;

  versionHeader.IDE = CAN_ID_STD;
  versionHeader.RTR = CAN_RTR_DATA;

  versionHeader.TransmitGlobalTime = DISABLE;

  uint32_t mailbox;

  // logging
  // HAL_UART_Transmit_IT(uart, (uint8_t *)"sending version\n", 16);

  // sending version
  return HAL_CAN_AddTxMessage(can, &versionHeader, version, &mailbox);
}

// CAN display system failure handle
// cmdNum 0x88
// id = 0x440
// DLC = 0 bytes
HAL_StatusTypeDef CMD_SysFail(CanRxMessage_t *msg) {
  HAL_UART_Transmit_IT(uart, (uint8_t *)"ERROR: SYSTEM FAILURE RECEIVED \n",
                       32);

  ILI9488_BlitImage(spi, 0, 0, &SYSFAIL_480x320, true
#if COLOUR_ENABLED
                    ,
                    COLOR_RED
#endif
  );
  return HAL_OK;
}

// CAN brightness change handle
// cmdNum 0x89
// id = 0x448
// DLC = 1 byte
// data format = BRIGHTNESS_FLAG
HAL_StatusTypeDef CMD_Brightness(CanRxMessage_t *msg) {

  // getting brightness flag to determine what to do to brightness
  uint8_t brightnessFlag = msg->data[0];
  uint32_t thisTick = HAL_GetTick();

  switch (brightnessFlag) {

  case 1:
    // decrement brightness

    // if brightness is already at its lowest index
    if (brightnessIdx == 0) {

      // start beep
      ALARM_StartBeep(alarmTimer);
      beepTick = thisTick;

    } else {
      // decrement

      brightnessIdx--;
    }
    break;

  case 2:
    // increment brightness

    // if brightness is already at its highest index
    if (brightnessIdx == BRIGHTNESS_TABLE_SIZE - 1) {
      // start beep
      ALARM_StartBeep(alarmTimer);
      beepTick = thisTick;

    } else {
      // increment brightness
      brightnessIdx++;
    }
    break;

  case 3:
    // reset brightness

    brightnessIdx = DEFAULT_BRIGHTNESS_INDEX;
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

// CAN set alarm handle
// cmdNum 0x8A
// id = 0x450
// DLC = 2 bytes
// data format = dutyCycle, frequency
HAL_StatusTypeDef CMD_Alarm(CanRxMessage_t *msg) {
  uint8_t dutyCycle = msg->data[0];
  uint8_t frequency = msg->data[1];

  // setting alarm settings
  // note: in the current implementation,
  // the duty cycle is used as a fraction of 255 to set duty cycle.
  // This may need to be changed to 50% all the time
  ALARM_Set(alarmTimer, frequency, dutyCycle);

  // logging
  // uint8_t len =
  //     snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg),
  //              "changed alarm to a frequency of %u and a duty of %u\n",
  //              frequency, dutyCycle);

  // HAL_UART_Transmit_IT(uart, diagnosticMsg, len);

  return HAL_OK;
}

// list of commands
static CanCommand_t commands[] = {

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

//--------------------------------------------------------------------------------
// public functions

// initialization function to be called in the main loop
HAL_StatusTypeDef
CAN_CMDS_Init(CAN_HandleTypeDef *canInterface,
              SPI_HandleTypeDef *displaySpiInterface,
              UART_HandleTypeDef *serialLoggingInterface,
              TIM_HandleTypeDef *alarmPWMTimerInterface,
              TIM_HandleTypeDef *backlightPWMTimerInterface,
              BrightnessInfo_t brightnessSettings, GPIO_TypeDef *baudInput1Port,
              uint16_t baudInput1Pin, GPIO_TypeDef *baudInput2Port,
              uint16_t baudInput2Pin, GPIO_TypeDef *baudInput3Port,
              uint16_t baudInput3Pin) {

  // setting global interfaces
  can = canInterface;
  spi = displaySpiInterface;
  uart = serialLoggingInterface;
  alarmTimer = alarmPWMTimerInterface;
  backlightTimer = backlightPWMTimerInterface;

  // last message tick reference
  lastMsgTick = HAL_GetTick();

  HAL_Delay(500);

  // setting brightness globals from inputted brightness settings
  flashOffset = brightnessSettings.flashOffset;
  prevBrightnessIdx = brightnessSettings.prevBrightnessIdx;

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

  // choosing lowest priority pin that's grounded as the can BAUD rate.
  // Adjust accordingly
  // Heirarchy:
  // baudInput3 (666.6kb baud)>
  // baudInput2 (500kb baud) 	>
  // baudInput1 (250kb baud) 	>
  // default (128kb baud)

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

// CAN main loop function that:
// - processes CAN message queue
// - checks for CAN inactivity for 4 seconds
// - stops beeps after 100ms
// - writes new brightness index to flash after 5 seconds
HAL_StatusTypeDef CAN_CMDS_Process(void) {

  // saving a copy of lastmsg tick to prevent overflow in case an interrupt
  // fires mid function
  uint32_t lastMsgTickCopy = lastMsgTick;
  // saving current tick in case interrupt fires mid function
  uint32_t currentTick = HAL_GetTick();

  // display error if the can bus is silent for 4 seconds
  if (currentTick - lastMsgTickCopy > 4000 && lastMsgTickCopy != 0) {

    // logging message
    HAL_UART_Transmit_IT(
        uart, (uint8_t *)"TIMEOUT: no command received in the last 4000ms\n",
        48);

    // display error image
    HAL_SPIN(ILI9488_BlitImage(spi, 0, 0, &SYSFAIL_480x320, true
#if COLOUR_ENABLED
                               ,
                               COLOR_RED
#endif

                               ));

    // resetting last message tick so the above condition doesn't trigger every
    // loop
    lastMsgTick = 0;
  }

  // if the beep has been on for 100 ms
  if (currentTick - beepTick >= 100 && beepTick != 0) {

    // logging
    // HAL_UART_Transmit_IT(uart, (uint8_t *)"Brightness beep completed\n",
    // 26);

    // stopping the beep
    ALARM_StopBeep(alarmTimer);

    // resetting beep tick
    beepTick = 0;
  }

  // if it's been 5 seconds since last brightness change,
  // and the new brightness value doesn't equal the last saved brightness value
  if (brightnessIdx != prevBrightnessIdx && brightnessTick != 0 &&
      currentTick - brightnessTick > 5000) {

    // resetting brightness tick
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

      HAL_StatusTypeDef eraseStatus = HAL_FLASHEx_Erase(&erase, &pageError);
      if (eraseStatus != HAL_OK) {
        // relock flash on failure
        HAL_FLASH_Lock();

        // logging message
        HAL_UART_Transmit_IT(uart,
                             (uint8_t *)"Failed to write brightness to flash "
                                        "(couldn't erase flash)\n",
                             59);
        return eraseStatus;
      }

      // reset flash offset when flash is erased
      flashOffset = 0;
    }

    // minimum flash write resolution is 16 bit (halfword)
    // making second nibble 0 to indicate a write for startup parsing
    uint16_t halfword = (uint16_t)brightnessIdx | 0x0000u;

    // writing the halfword to the next empty slot in flash
    HAL_StatusTypeDef writeStatus =
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                          BRIGHTNESS_PAGE_ADDR + flashOffset, halfword);
    HAL_FLASH_Lock();

    if (writeStatus != HAL_OK) {
      // logging message
      HAL_UART_Transmit_IT(
          uart,
          (uint8_t *)"Failed to write brightness to flash (write failure)\n",
          59);

      return writeStatus;
    }

    // incrementing offset by 2 (because each half word stores 2 bytes)
    flashOffset += 2;

    prevBrightnessIdx = brightnessIdx;

    // diagnostic logging
    HAL_UART_Transmit_IT(
        uart, (uint8_t *)"Successfully wrote brightness to flash\n", 39);
  }

  // logging the fill level of the queue
  // if (readIdx != writeIdx) {
  //   // logging
  //   uint8_t len = snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg),
  //                          "%u, %u\n", readIdx, writeIdx);
  //
  //   HAL_UART_Transmit_IT(uart, diagnosticMsg, len);
  // }

  // iterating through every message
  while (readIdx != writeIdx) {

    // reference to current message
    CanRxMessage_t *msg = &queue[readIdx];

    // command number
    uint8_t cmdNum = (uint8_t)(msg->header.StdId >> 3);

    // assuming the commandnums are contiguous and in the correct order
    // if the command number is in range of the list of commands
    if (cmdNum >= commands[0].cmdNum &&
        cmdNum <=
            commands[0].cmdNum + (sizeof(commands) / sizeof(commands[0])) - 1) {

      // executing handle for command
      commands[cmdNum - commands[0].cmdNum].handle(msg);

    } else {
      // if no commands match (should never trigger because of the CAN filter)
      // increment read index mod queue size
      readIdx = (readIdx + 1) & (QUEUE_SIZE - 1);

      // return error (the rest of the commands will process next loop)
      return HAL_ERROR;
    }

    // increment read index mod queue size
    readIdx = (readIdx + 1) & (QUEUE_SIZE - 1);
  }

  return HAL_OK;
}

//--------------------------------------------------------------------------------
// Interrupts/callbacks

// callback for received message
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  // setting last tick
  lastMsgTick = HAL_GetTick();

  // incrementing the next write index mod the size of buffer
  // note: The & optimization only works if QUEUE_SIZE is a power of 2
  uint8_t nextWriteIdx = (writeIdx + 1) & (QUEUE_SIZE - 1);

  // when the write idx has almost "lapped" the read index
  if (nextWriteIdx == readIdx) {
    // ON QUEUE OVERFLOW - write message to junk buffer to discard it
    CAN_RxHeaderTypeDef hdr;
    uint8_t data[8];
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &hdr, data);
  } else {
    // message to next spot in queue
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &queue[writeIdx].header,
                         queue[writeIdx].data);
  }

  // incrementing writeidx
  writeIdx = nextWriteIdx;
}

// callback for junk/heartbeat messages to check if network is still active
void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  // setting last tick
  lastMsgTick = HAL_GetTick();
  // temporary variable allocation to store junk message
  CAN_RxHeaderTypeDef hdr;
  uint8_t data[8];

  HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &hdr, data);
}
