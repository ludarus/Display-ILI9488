/*
 * commands-can.c
 *
 *  Created on: Jun 30, 2026
 *      Author: Luke Fadel
 */

#include "commands-can.h"
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
#include <stdbool.h>
#include <stdio.h>

// includes for every image
#include "File_002_ObjNum_001_NEW_6_17_26.h"
#include "File_005_ObjNum_004_480x320_6_18_26.h"
#include "File_006_ObjNum_005_480x320_6_18_26_C.h"
#include "File_007_ObjNum_006_480x320_6_18_26.h"
#include "File_008_ObjNum_007_96x190_6_18_26.h"
#include "File_009_ObjNum_008_96x190_6_18_26.h"
#include "File_010_ObjNum_009_96x190_6_18_26.h"
#include "File_011_ObjNum_010_96x190_6_18_26.h"
#include "File_012_ObjNum_011_96x190_6_18_26.h"
#include "File_013_ObjNum_012_96x190_6_18_26.h"
#include "File_014_ObjNum_013_96x190_6_18_26.h"
#include "File_015_ObjNum_014_96x190_6_18_26.h"
#include "File_016_ObjNum_015_96x190_6_18_26.h"
#include "File_017_ObjNum_016_96x190_6_18_26.h"
#include "File_018_ObjNum_017_96x190_6_16_26.h"
#include "File_019_ObjNum_018_48x208_6_19_26.h"
#include "File_020_ObjNum_019_48x208_6_19_26.h"
#include "File_021_ObjNum_020_48x208_6_19_26.h"
#include "File_022_ObjNum_021_48x208_6_19_26.h"
#include "File_023_ObjNum_022_48x208_6_19_26.h"
#include "File_024_ObjNum_023_48x208_6_19_26.h"
#include "File_025_ObjNum_024_48x208_6_19_26.h"
#include "File_026_ObjNum_025_48x208_6_19_26.h"
#include "File_027_ObjNum_026_48x208_6_19_26.h"
#include "File_028_ObjNum_027_48x208_6_19_26.h"
#include "File_029_ObjNum_028_48x208_6_19_26.h"
#include "File_030_ObjNum_029_64x25_6_19_26.h"
#include "File_031_ObjNum_030_64x25_6_19_26.h"
#include "File_032_ObjNum_031_96x190_6_19_26.h"
#include "File_033_ObjNum_032_96x190_6_19_26.h"
#include "File_034_ObjNum_033_96x190_6_19_26.h"
#include "File_035_ObjNum_034_96x190_6_19_26.h"
#include "File_036_ObjNum_035_96x190_6_19_26.h"
#include "File_037_ObjNum_036_96x190_6_19_26.h"
#include "File_038_ObjNum_037_96x190_6_19_26.h"
#include "File_039_ObjNum_038_96x190_6_19_26.h"
#include "File_040_ObjNum_039_96x190_6_19_26.h"
#include "File_041_ObjNum_040_96x190_6_19_26.h"
#include "File_042_ObjNum_041_96x190_6_19_26.h"
#include "File_043_ObjNum_042_144x208_6_19_26.h"
#include "File_044_ObjNum_043_144x208_6_19_26.h"
#include "File_045_ObjNum_044_144x208_6_19_26.h"
#include "File_046_ObjNum_045_144x208_6_19_26.h"
#include "File_047_ObjNum_046_144x208_6_19_26.h"
#include "File_048_ObjNum_052_144x183_6_19_26.h"
#include "File_049_ObjNum_053_144x183_6_19_26.h"
#include "File_050_ObjNum_054_144x183_6_19_26.h"
#include "File_051_ObjNum_055_144x183_6_19_26.h"
#include "File_052_ObjNum_057_144x30_6_19_26.h"
#include "File_053_ObjNum_077_48x30_6_19_26.h"
#include "File_054_ObjNum_087_48x255_6_19_26.h"
#include "File_055_ObjNum_090_480x320_6_18_26.h"
#include "File_056_ObjNum_091_480x320_6_18_26.h"
#include "File_057_ObjNum_092_480x320_6_18_26.h"
#include "File_058_ObjNum_093_480x320_6_18_26.h"
#include "File_059_ObjNum_094_480x320_6_18_26.h"
#include "File_062_ObjNum_097_480x40_6_19_26.h"
#include "File_063_ObjNum_098_176x33_6_19_26.h"
#include "File_064_ObjNum_099_48x63_6_19_26.h"
#include "File_065_ObjNum_104_16x30_6_19_26.h"
#include "File_066_ObjNum_109_96x80_6_19_26.h"
#include "File_067_ObjNum_114_64x103_6_19_26.h"
#include "File_068_ObjNum_119_64x103_6_19_26.h"
#include "File_069_ObjNum_124_64x103_6_19_26.h"
#include "File_070_ObjNum_129_96x80_6_19_26.h"
#include "File_071_ObjNum_134_176x188_6_19_26.h"
#include "File_072_ObjNum_135_480x320_6_18_26.h"
#include "File_073_ObjNum_136_480x320_6_18_26.h"
#include "File_074_ObjNum_138_48x143_6_19_26.h"
#include "File_075_ObjNum_140_368x40_6_19_26.h"
#include "File_076_ObjNum_143_368x40_6_19_26.h"
#include "File_077_ObjNum_147_480x320_6_18_26.h"
#include "File_078_ObjNum_148_480x320_6_18_26.h"
#include "File_079_ObjNum_149_480x320_6_17_26.h"
#include "SYSFAIL_480x320.h"

static const Obj_t objects[] = {
    {1, 0, 0, 0, &File_002_ObjNum_001_NEW_6_17_26},
    {
        2,
        3,
        0,
        0,
    },
    {
        3,
        9,
        0,
        0,
    },
    {4, 0, 0, 0, &File_005_ObjNum_004_480x320_6_18_26},
    {5, 0, 0, 0, &File_006_ObjNum_005_480x320_6_18_26_C},
    {6, 0, 0, 0, &File_007_ObjNum_006_480x320_6_18_26},
    {7, 3, 0, 53, &File_008_ObjNum_007_96x190_6_18_26},
    {8, 3, 0, 53, &File_009_ObjNum_008_96x190_6_18_26},
    {9, 3, 0, 53, &File_010_ObjNum_009_96x190_6_18_26},
    {10, 3, 0, 53, &File_011_ObjNum_010_96x190_6_18_26},
    {11, 3, 0, 53, &File_012_ObjNum_011_96x190_6_18_26},
    {12, 3, 0, 53, &File_013_ObjNum_012_96x190_6_18_26},
    {13, 3, 0, 53, &File_014_ObjNum_013_96x190_6_18_26},
    {14, 3, 0, 53, &File_015_ObjNum_014_96x190_6_18_26},
    {15, 3, 0, 53, &File_016_ObjNum_015_96x190_6_18_26},
    {16, 3, 0, 53, &File_017_ObjNum_016_96x190_6_18_26},
    {17, 3, 0, 53, &File_018_ObjNum_017_96x190_6_16_26},
    {18, 3, 432, 45, &File_019_ObjNum_018_48x208_6_19_26},
    {19, 3, 432, 45, &File_020_ObjNum_019_48x208_6_19_26},
    {20, 3, 432, 45, &File_021_ObjNum_020_48x208_6_19_26},
    {21, 3, 432, 45, &File_022_ObjNum_021_48x208_6_19_26},
    {22, 3, 432, 45, &File_023_ObjNum_022_48x208_6_19_26},
    {23, 3, 432, 45, &File_024_ObjNum_023_48x208_6_19_26},
    {24, 3, 432, 45, &File_025_ObjNum_024_48x208_6_19_26},
    {25, 3, 432, 45, &File_026_ObjNum_025_48x208_6_19_26},
    {26, 3, 432, 45, &File_027_ObjNum_026_48x208_6_19_26},
    {27, 3, 432, 45, &File_028_ObjNum_027_48x208_6_19_26},
    {28, 3, 432, 45, &File_029_ObjNum_028_48x208_6_19_26},
    {29, 3, 408, 45, &File_030_ObjNum_029_64x25_6_19_26},
    {30, 3, 408, 45, &File_031_ObjNum_030_64x25_6_19_26},
    {31, 3, 0, 53, &File_032_ObjNum_031_96x190_6_19_26},
    {32, 3, 0, 53, &File_033_ObjNum_032_96x190_6_19_26},
    {33, 3, 0, 53, &File_034_ObjNum_033_96x190_6_19_26},
    {34, 3, 0, 53, &File_035_ObjNum_034_96x190_6_19_26},
    {35, 3, 0, 53, &File_036_ObjNum_035_96x190_6_19_26},
    {36, 3, 0, 53, &File_037_ObjNum_036_96x190_6_19_26},
    {37, 3, 0, 53, &File_038_ObjNum_037_96x190_6_19_26},
    {38, 3, 0, 53, &File_039_ObjNum_038_96x190_6_19_26},
    {39, 3, 0, 53, &File_040_ObjNum_039_96x190_6_19_26},
    {40, 3, 0, 53, &File_041_ObjNum_040_96x190_6_19_26},
    {41, 3, 0, 53, &File_042_ObjNum_041_96x190_6_19_26},
    {42, 3, 96, 45, &File_043_ObjNum_042_144x208_6_19_26},
    {43, 3, 96, 45, &File_044_ObjNum_043_144x208_6_19_26},
    {44, 3, 96, 45, &File_045_ObjNum_044_144x208_6_19_26},
    {45, 3, 96, 45, &File_046_ObjNum_045_144x208_6_19_26},
    {46, 3, 96, 45, &File_047_ObjNum_046_144x208_6_19_26},
    {47, 3, 96, 45, &File_043_ObjNum_042_144x208_6_19_26},
    {48, 3, 96, 45, &File_044_ObjNum_043_144x208_6_19_26},
    {49, 3, 96, 45, &File_045_ObjNum_044_144x208_6_19_26},
    {50, 3, 96, 45, &File_046_ObjNum_045_144x208_6_19_26},
    {51, 3, 96, 45, &File_047_ObjNum_046_144x208_6_19_26},
    {52, 3, 96, 45, &File_048_ObjNum_052_144x183_6_19_26},
    {53, 3, 96, 45, &File_049_ObjNum_053_144x183_6_19_26},
    {54, 3, 96, 45, &File_050_ObjNum_054_144x183_6_19_26},
    {55, 3, 96, 45, &File_051_ObjNum_055_144x183_6_19_26},
    {56, 3, 96, 45, &File_049_ObjNum_053_144x183_6_19_26},
    {57, 3, 240, 45, &File_052_ObjNum_057_144x30_6_19_26},
    {
        58,
        1,
        0,
        0,
    },
    {
        59,
        1,
        240,
        0,
    },
    {
        60,
        1,
        408,
        0,
    },
    {
        61,
        1,
        0,
        266,
    },
    {
        62,
        1,
        264,
        266,
    },
    {
        63,
        1,
        0,
        0,
    },
    {
        64,
        1,
        0,
        76,
    },
    {
        65,
        1,
        0,
        152,
    },
    {
        66,
        1,
        0,
        228,
    },
    {
        67,
        1,
        48,
        57,
    },
    {
        68,
        1,
        48,
        106,
    },
    {
        69,
        1,
        48,
        156,
    },
    {
        70,
        1,
        48,
        205,
    },
    {
        71,
        1,
        48,
        255,
    },
    {
        72,
        1,
        288,
        57,
    },
    {
        73,
        1,
        288,
        106,
    },
    {
        74,
        1,
        288,
        156,
    },
    {
        75,
        1,
        288,
        205,
    },
    {
        76,
        1,
        288,
        255,
    },
    {77, 3, 0, 60, &File_053_ObjNum_077_48x30_6_19_26},
    {78, 3, 0, 110, &File_053_ObjNum_077_48x30_6_19_26},
    {79, 3, 0, 160, &File_053_ObjNum_077_48x30_6_19_26},
    {80, 3, 0, 209, &File_053_ObjNum_077_48x30_6_19_26},
    {81, 3, 0, 259, &File_053_ObjNum_077_48x30_6_19_26},
    {82, 3, 240, 60, &File_053_ObjNum_077_48x30_6_19_26},
    {83, 3, 240, 110, &File_053_ObjNum_077_48x30_6_19_26},
    {84, 3, 240, 160, &File_053_ObjNum_077_48x30_6_19_26},
    {85, 3, 240, 209, &File_053_ObjNum_077_48x30_6_19_26},
    {86, 3, 240, 259, &File_053_ObjNum_077_48x30_6_19_26},
    {87, 3, 0, 45, &File_054_ObjNum_087_48x255_6_19_26},
    {88, 3, 240, 45, &File_054_ObjNum_087_48x255_6_19_26},
    {
        89,
        1,
        0,
        228,
    },
    {90, 3, 0, 0, &File_055_ObjNum_090_480x320_6_18_26},
    {91, 3, 0, 0, &File_056_ObjNum_091_480x320_6_18_26},
    {92, 3, 0, 0, &File_057_ObjNum_092_480x320_6_18_26},
    {93, 3, 0, 0, &File_058_ObjNum_093_480x320_6_18_26},
    {94, 3, 0, 0, &File_059_ObjNum_094_480x320_6_18_26},
    {
        95,
        4,
        0,
        0,
    },
    {
        96,
        4,
        0,
        0,
    },
    {97, 3, 0, 232, &File_062_ObjNum_097_480x40_6_19_26},
    {98, 3, 240, 190, &File_063_ObjNum_098_176x33_6_19_26},
    {99, 3, 264, 220, &File_064_ObjNum_099_48x63_6_19_26},
    {100, 3, 288, 220, &File_064_ObjNum_099_48x63_6_19_26},
    {101, 3, 312, 220, &File_064_ObjNum_099_48x63_6_19_26},
    {102, 3, 336, 220, &File_064_ObjNum_099_48x63_6_19_26},
    {103, 3, 360, 220, &File_064_ObjNum_099_48x63_6_19_26},
    {104, 3, 264, 220, &File_065_ObjNum_104_16x30_6_19_26},
    {105, 3, 288, 220, &File_065_ObjNum_104_16x30_6_19_26},
    {106, 3, 312, 220, &File_065_ObjNum_104_16x30_6_19_26},
    {107, 3, 336, 220, &File_065_ObjNum_104_16x30_6_19_26},
    {108, 3, 360, 220, &File_065_ObjNum_104_16x30_6_19_26},
    {109, 3, 240, 110, &File_066_ObjNum_109_96x80_6_19_26},
    {110, 3, 264, 110, &File_066_ObjNum_109_96x80_6_19_26},
    {111, 3, 288, 110, &File_066_ObjNum_109_96x80_6_19_26},
    {112, 3, 312, 110, &File_066_ObjNum_109_96x80_6_19_26},
    {113, 3, 336, 110, &File_066_ObjNum_109_96x80_6_19_26},
    {114, 3, 240, 87, &File_067_ObjNum_114_64x103_6_19_26},
    {115, 3, 264, 87, &File_067_ObjNum_114_64x103_6_19_26},
    {116, 3, 288, 87, &File_067_ObjNum_114_64x103_6_19_26},
    {117, 3, 312, 87, &File_067_ObjNum_114_64x103_6_19_26},
    {118, 3, 336, 87, &File_067_ObjNum_114_64x103_6_19_26},
    {119, 3, 240, 87, &File_068_ObjNum_119_64x103_6_19_26},
    {120, 3, 264, 87, &File_068_ObjNum_119_64x103_6_19_26},
    {121, 3, 288, 87, &File_068_ObjNum_119_64x103_6_19_26},
    {122, 3, 312, 87, &File_068_ObjNum_119_64x103_6_19_26},
    {123, 3, 336, 87, &File_068_ObjNum_119_64x103_6_19_26},
    {124, 3, 240, 87, &File_069_ObjNum_124_64x103_6_19_26},
    {125, 3, 264, 87, &File_069_ObjNum_124_64x103_6_19_26},
    {126, 3, 288, 87, &File_069_ObjNum_124_64x103_6_19_26},
    {127, 3, 312, 87, &File_069_ObjNum_124_64x103_6_19_26},
    {128, 3, 336, 87, &File_069_ObjNum_124_64x103_6_19_26},
    {129, 3, 240, 110, &File_070_ObjNum_129_96x80_6_19_26},
    {130, 3, 264, 110, &File_070_ObjNum_129_96x80_6_19_26},
    {131, 3, 288, 110, &File_070_ObjNum_129_96x80_6_19_26},
    {132, 3, 312, 110, &File_070_ObjNum_129_96x80_6_19_26},
    {133, 3, 336, 110, &File_070_ObjNum_129_96x80_6_19_26},
    {134, 3, 240, 83, &File_071_ObjNum_134_176x188_6_19_26},
    {135, 0, 0, 0, &File_072_ObjNum_135_480x320_6_18_26},
    {136, 0, 0, 0, &File_073_ObjNum_136_480x320_6_18_26},
    {
        137,
        1,
        144,
        220,
    },
    {138, 3, 0, 160, &File_074_ObjNum_138_48x143_6_19_26},
    {139, 3, 240, 160, &File_074_ObjNum_138_48x143_6_19_26},
    {140, 3, 0, 167, &File_075_ObjNum_140_368x40_6_19_26},
    {141, 3, 0, 213, &File_075_ObjNum_140_368x40_6_19_26},
    {142, 3, 0, 259, &File_075_ObjNum_140_368x40_6_19_26},
    {143, 3, 0, 167, &File_076_ObjNum_143_368x40_6_19_26},
    {144, 3, 0, 213, &File_076_ObjNum_143_368x40_6_19_26},
    {145, 3, 0, 259, &File_076_ObjNum_143_368x40_6_19_26},
    {
        146,
        1,
        96,
        228,
    },
    {147, 0, 0, 0, &File_077_ObjNum_147_480x320_6_18_26},
    {148, 0, 0, 0, &File_078_ObjNum_148_480x320_6_18_26},
    {149, 0, 0, 0, &File_079_ObjNum_149_480x320_6_17_26},
};

// TODO get correct version string
const uint8_t version[8] = "DSP12345";

// debugging flag
volatile static bool overFlowed = false;

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
static uint32_t alarmTick;

// for error
static uint32_t lastMsgTick = 1;

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
      // diagnostic
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
  Image_t *bg = (Image_t *)(*obj).img;

  // if it's not a background type or it has no image, SET background to blank
  if ((*obj).type != 0 || (*obj).img == NULL) {
    bg = (Image_t *)&File_005_ObjNum_004_480x320_6_18_26;
  }

  HAL_TRY(ILI9488_SetBackground(bg));

  HAL_SPIN(ILI9488_LoadImage(spi, 0, 0, bg, true, false, true));

  // display according image
  uint8_t len = snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg),
                         "displayed background with objNum: %u\n", objNum);

  HAL_UART_Transmit_IT(uart, diagnosticMsg, len);

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
    uint8_t First_Pkt_Flag = msg->data[0];
    // number of characters in the string
    remainingChars = msg->data[1];
    target = remainingChars;
    // obj number
    uint8_t lsb = msg->data[2];
    uint16_t msb = msg->data[3] << 8;

    objNum = lsb | msb;

    // filling up the 4 bytes of charInfo contained within this packet
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

    uint8_t len = snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg),
                           "Disp text: \"%.*s\", objNum = %u\n", target,
                           charArray, objNum);

    HAL_UART_Transmit_IT(uart, diagnosticMsg, len);

    // restting target
    target = 0;
    // return displayStatus;
    return HAL_OK;
  }

  // logging
  HAL_UART_Transmit_IT(uart, (uint8_t *)"recieved partial text packet\n", 29);

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

  // if it's not an image type or it has no image
  if ((*obj).type != 3 || (*obj).img == NULL) {
    return HAL_ERROR;
  }

  // display according image
  HAL_SPIN(ILI9488_LoadImage(spi, (*obj).x, (*obj).y, (*obj).img, true, false,
                             true));

  // diagnostic
  uint8_t len = snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg),
                         "displayed image with objNum: %u\n", objNum);

  HAL_UART_Transmit_IT(uart, diagnosticMsg, len);

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

  // display according image
  // ILI9488_LOAD_IMAGE(spi, uint16_t x, uint16_t y, const Image_t *image, bool
  // overWrite, bool draw)

  // diagnostic
  uint8_t len = snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg),
                         "displayed group with grpNum: %u and index: %u\n",
                         grpNum, index);

  HAL_UART_Transmit_IT(uart, diagnosticMsg, len);

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

  HAL_UART_Transmit_IT(uart, (uint8_t *)"sending version\n", 16);

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

  // diagnostic
  uint8_t len = snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg),
                         "changed display brightness to %u\n", brightnessVal);

  HAL_UART_Transmit_IT(uart, diagnosticMsg, len);

  return HAL_OK;
}

HAL_StatusTypeDef CMD_Alarm(CanRxMessage_t *msg) {
  // assuming dutyCycle is an 8 bit number where 0 is off and 255 is 100%
  uint8_t dutyCycle = msg->data[0];
  uint8_t frequency = msg->data[1];

  ALARM_Set(alarmTimer, frequency, dutyCycle);

  uint8_t len =
      snprintf((char *)diagnosticMsg, sizeof(diagnosticMsg),
               "changed alarm to a frequency of %u and a duty of %u\n",
               frequency, dutyCycle);

  HAL_UART_Transmit_IT(uart, diagnosticMsg, len);

  return HAL_OK;
}

// list of commands to be received
static const CanCommand_t commands[] = {

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

  // starting device
  HAL_TRY(HAL_CAN_Start(canInterface));
  // enabling interrupt for can receive
  HAL_TRY(
      HAL_CAN_ActivateNotification(canInterface, CAN_IT_RX_FIFO0_MSG_PENDING));

  return HAL_OK;
}

HAL_StatusTypeDef CAN_CMDS_Process(void) {

  // display error if the can bus is silent for 4 seconds
  if (HAL_GetTick() - lastMsgTick > 4000 && lastMsgTick != 0) {
    // display error image.
    HAL_UART_Transmit_IT(
        uart, (uint8_t *)"TIMEOUT: no command received in the last 4000ms\n",
        48);

    // ILI9488_LOAD_IMAGE(spi, 0, 0, &SYSFAIL_480x320, true, true);

    lastMsgTick = 0;
  }

  // if the alarm has been on for 100 ms
  if (HAL_GetTick() - alarmTick >= 100 && alarmTick != 0) {

    HAL_UART_Transmit_IT(uart, (uint8_t *)"Brightness beep completed\n", 26);

    ALARM_StopBeep(alarmTimer);

    alarmTick = 0;
  }

  // if it's been 5 seconds since last brightness change
  if (brightnessVal != prevBrightnessVal && brightnessTick != 0 &&
      HAL_GetTick() - brightnessTick > 5000) {
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

    // diagnostic
    HAL_UART_Transmit_IT(
        uart, (uint8_t *)"Successfully wrote brightness to flash\n", 39);
  }

  // iterating through every message
  uint8_t snapshot = queuedMessages;
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
      HAL_TRY(commands[cmdNum - commands[0].cmdNum].handle(&queue[msgIdx]));

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
  if (queuedMessages < 48) {
    // no error handling in interrupt. Possibly do something to fix this
    HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &queue[queuedMessages].header,
                         queue[queuedMessages].data);
    queuedMessages++;
    lastMsgTick = HAL_GetTick();
  } else {
    overFlowed = true;
  }
}
