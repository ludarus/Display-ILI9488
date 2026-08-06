/*
 * display.c
 *
 *  Created on: Jun 23, 2026
 *      Author: Luke Fadel
 *
 *  Created for ILI9488 display driver
 *  https://www.mouser.com/pdfDocs/ILI9488_Data_Sheet_100.pdf
 *  LSB = first pixel in byte
 *
 *      Type annotations: _p = in pixels, _b = in bytes
 */

// includes #include "character.h" #include "image.h"
#include "File_005_ObjNum_004_480x320_6_18_26.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_def.h"
#include "stm32f0xx_hal_gpio.h"
#include "stm32f0xx_hal_spi.h"
#include "stm32f0xx_hal_tim.h"
#include "tables.h"
#include <display-ili9488.h>
#include <stdbool.h>
#include <stdint.h>

// state so main functions and callbacks can all access render state
static ImageTransferState_t state;

//--------------------------------------------------------------------------------
// private inline utility functions: ----

// expands a bitpacked byte to the proper data format & colour resolution to
// transmit to the display.
// Takes 1 byte which contains 8 pixels worth of information and expands it into
// 4 bytes of colour coded data
static inline void expandToChunk(uint8_t *screenData, uint32_t *dst,
                                 const uint32_t count) {
  uint32_t pos = state.fillPos;
  uint16_t col = state.fillCol;
  const uint16_t width = state.width;
  const uint16_t rowSkip = state.rowSkip;

  for (uint32_t i = 0; i < count; i++) {
    dst[i] = pixelTable[screenData[pos]];
    pos++;
    if (++col == width) {
      col = 0;
      pos += rowSkip;
    }
  }

  state.fillPos = pos;
  state.fillCol = col;
}

// walks through RLE encoded image
static inline void rleAdvance(const uint8_t *data, uint32_t *remaining,
                              uint32_t *index, uint32_t count) {
  // traversing RLE stream to get to targetted value
  while (count > 0 || *remaining == 0) {
    if (*remaining > count) {
      *remaining -= count;
      return;
    }
    count -= *remaining;

    (*index)++;
    *remaining = data[*index];
  }
}

// bitpacks a contiguous section of a buffer while advancing position pointer
static inline void fillScreen(uint8_t *buffer, uint32_t *position,
                              uint16_t count, const bool isOn,
                              const bool overWrite) {
  // leading bits
  // if the current position isn't byte aligned
  if ((*position) % 8 != 0) {
    uint8_t offset = (*position) % 8;
    uint8_t leading = 8 - offset;
    leading = leading > count ? count : leading;
    uint8_t mask = (uint8_t)(((1u << leading) - 1u) << offset);
    if (isOn) {
      buffer[(*position) >> 3] |= mask;
    } else if (overWrite) {
      buffer[(*position) >> 3] &= (uint8_t)~mask;
    }
    count -= leading;
    (*position) += leading;
  }

  // middle bytes
  for (uint8_t byte = 0; byte < count >> 3; byte++) {
    if (isOn) {
      // write byte
      buffer[(*position) >> 3] = 0xFF;
    } else if (overWrite) {
      // clear byte
      buffer[(*position) >> 3] = 0;
    }

    // incrementing global position
    (*position) += 8;
  }

  // trailing bits
  uint8_t trailing = count % 8;
  // check to make sure there's unaligned bits
  if (trailing != 0) {
    if (isOn) {
      buffer[(*position) >> 3] |= 0xFF >> (8 - trailing);
    } else if (overWrite) {
      buffer[(*position) >> 3] &= 0xFF << trailing;
    }
    (*position) += trailing;
  }
}

static inline void bgPreload(const uint8_t *bgData, uint8_t *screenData,
                             uint32_t position, const uint16_t width,
                             const uint16_t height) {

  const uint16_t rowSkip = ILI9488_WIDTH_PX - width;

  uint32_t bgRem = bgData[0]; // in pixels
  uint32_t bgIdx = 0;

  // getting correct background index for rle value
  rleAdvance(bgData, &bgRem, &bgIdx, position);

  for (uint16_t row = 0; row < height; row++) {
    uint16_t remainingPx = width;

    while (remainingPx > 0) {
      uint16_t chunk = (remainingPx > bgRem) ? bgRem : remainingPx;
      remainingPx -= chunk;

      fillScreen(screenData, &position, chunk, bgIdx % 2, true);

      rleAdvance(bgData, &bgRem, &bgIdx, chunk);
    }
    rleAdvance(bgData, &bgRem, &bgIdx, rowSkip);
    position += rowSkip;
  }
}

// private display functions

// LCD hardware Reset, active low
void ILI9488_Reset(void) {
  // setting the reset pin to low to signal a reset
  HAL_GPIO_WritePin(DISPLAY_RESET_GPIO_Port, DISPLAY_RESET_Pin, GPIO_PIN_RESET);

  // small delay
  HAL_Delay(10);

  // setting the pin to high (default state)
  HAL_GPIO_WritePin(DISPLAY_RESET_GPIO_Port, DISPLAY_RESET_Pin, GPIO_PIN_SET);

  HAL_Delay(100);
}

// LCD chip select signal, active low
void ILI9488_Select(void) {
  // setting the select pin to low
  HAL_GPIO_WritePin(DISPLAY_CS_GPIO_Port, DISPLAY_CS_Pin, GPIO_PIN_RESET);
}

void ILI9488_Deselect(void) {
  // setting the select pin to high
  HAL_GPIO_WritePin(DISPLAY_CS_GPIO_Port, DISPLAY_CS_Pin, GPIO_PIN_SET);
}

// sends command to controller
HAL_StatusTypeDef ILI9488_Cmd(SPI_HandleTypeDef *spi, uint8_t cmd) {
  // setting DC pin to command mode (low)
  HAL_GPIO_WritePin(DISPLAY_DC_GPIO_Port, DISPLAY_DC_Pin, GPIO_PIN_RESET);

  // selecting SPI device
  ILI9488_Select();

  // using SPI to transmit data
  HAL_StatusTypeDef status = HAL_SPI_Transmit(spi, &cmd, 1, HAL_MAX_DELAY);

  // deselecting SPI device
  ILI9488_Deselect();

  return status;
}

// sends data to controller
HAL_StatusTypeDef ILI9488_Data(SPI_HandleTypeDef *spi, uint8_t *data,
                               uint16_t size) {
  // setting DC pin to command mode (high)
  HAL_GPIO_WritePin(DISPLAY_DC_GPIO_Port, DISPLAY_DC_Pin, GPIO_PIN_SET);

  // selecting SPI device
  ILI9488_Select();

  // using SPI to transmit data
  HAL_StatusTypeDef status = HAL_SPI_Transmit(spi, data, size, HAL_MAX_DELAY);

  // deselecting SPI device
  ILI9488_Deselect();

  return status;
}

// utility function to set the writing range of the controller
HAL_StatusTypeDef ILI9488_SetRange(SPI_HandleTypeDef *spi, uint16_t colStart,
                                   uint16_t colEnd, uint16_t rowStart,
                                   uint16_t rowEnd) {
  // set column address command
  HAL_TRY(ILI9488_Cmd(spi, 0x2A));

  // parameters: starting col MSB, starting col LSB, ending col MSB, ending
  // col LSB
  uint8_t caset[] = {

      (uint8_t)(colStart >> 8),

      (uint8_t)(colStart & 0xFF),

      (uint8_t)(colEnd >> 8),

      (uint8_t)(colEnd & 0xFF)

  };

  HAL_TRY(ILI9488_Data(spi, caset, 4));

  // set row address command
  HAL_TRY(ILI9488_Cmd(spi, 0x2B));
  // parameters: starting row MSB, starting row LSB, ending row MSB, ending
  // row LSB
  uint8_t raset[] = {

      (uint8_t)(rowStart >> 8),

      (uint8_t)(rowStart & 0xFF),

      (uint8_t)(rowEnd >> 8),

      (uint8_t)(rowEnd & 0xFF)};

  HAL_TRY(ILI9488_Data(spi, raset, 4));

  return HAL_OK;
}

//--------------------------------------------------------------------------------
// public functions

// sets brightness of display
HAL_StatusTypeDef ILI9488_SetBrightness(SPI_HandleTypeDef *spi,
                                        TIM_HandleTypeDef *tim, uint8_t idx) {

  uint8_t val = brightnessTable[idx];
  // for pwm driven brightness
  __HAL_TIM_SET_COMPARE(tim, TIM_CHANNEL_1, val);

  // for displays without a physical brightness pin
  HAL_TRY(ILI9488_Cmd(spi, 0x51));
  HAL_TRY(ILI9488_Data(spi, &val, 1));

  return HAL_OK;
}

// setter for background image
HAL_StatusTypeDef ILI9488_SetBackground(const Image_t *bg) {
  // making sure requested image is full screen
  if (bg->height != 320 || bg->width != 480) {
    return HAL_ERROR;
  }

  state.backgroundImage = (Image_t *)bg;

  return HAL_OK;
}

// initializes the ILI9488
HAL_StatusTypeDef ILI9488_Init(SPI_HandleTypeDef *spi,
                               TIM_HandleTypeDef *backlightTimer) {

  // initializing background to empty image
  ILI9488_SetBackground((Image_t *)&File_005_ObjNum_004_480x320_6_18_26);

  // safety delays. can likely be removed
  HAL_Delay(200);

  // hardware reset
  ILI9488_Reset();
  HAL_Delay(200);

  // software reset
  HAL_TRY(ILI9488_Cmd(spi, 0x01));
  HAL_Delay(120);

  // exit sleep mode
  HAL_TRY(ILI9488_Cmd(spi, 0x11));
  HAL_Delay(120);

  // powering testing switches
  HAL_GPIO_WritePin(SWITCH_POWER_GPIO_Port, SWITCH_POWER_Pin, GPIO_PIN_SET);

  // backlight on
  // starting display backlight pwm timer
  HAL_TIM_PWM_Start(backlightTimer, TIM_CHANNEL_1);

  // configuring extended command set for spi write
  HAL_TRY(ILI9488_Cmd(spi, 0xF7));
  uint8_t unlockData[] = {0xA9, 0x51, 0x2C, 0x82};
  HAL_TRY(ILI9488_Data(spi, &unlockData[0], 2));

  // memory data access control - instruction 36h MADCTL
  HAL_TRY(ILI9488_Cmd(spi, 0x36));
  // same as st7796s
  // 0b00101001
  uint8_t madctl = 0x28;
  HAL_TRY(ILI9488_Data(spi, &madctl, 1));

  // configuring brightness control settings - instruction 53h WRCTRLD
  HAL_TRY(ILI9488_Cmd(spi, 0x53));
  // 0 0 1 0 1 1 0 0
  uint8_t brightnessCtl = 0x2C;
  HAL_TRY(ILI9488_Data(spi, &brightnessCtl, 1));

  // Interface Pixel Format - instruction 3Ah COLMOD
  HAL_TRY(ILI9488_Cmd(spi, 0x3A));
  // Lowest available is 3bit/pixel
  // 00000001
  // format: R G B 0 R G B 0
  // each byte = two pixels due to padding
  uint8_t colmod = 0x01;
  HAL_TRY(ILI9488_Data(spi, &colmod, 1));

  // Column inversion for display longevity
  // update: seems to cause flicker on this display?
  // 1 dot mode
  //	ILI9488_CMD(spi, 0xB4);
  //	uint8_t inversion = 0x01;
  //	ILI9488_DATA(spi, &inversion, 1);

  // enabling display inversion for IPS display
  HAL_TRY(ILI9488_Cmd(spi, 0x21));
  // no data

  // display on
  HAL_TRY(ILI9488_Cmd(spi, 0x29));

  return HAL_OK;
}

// loads image to screen buffer
HAL_StatusTypeDef ILI9488_LoadImage(SPI_HandleTypeDef *spi, uint16_t x_p,
                                    uint16_t y_p, const Image_t *image,
                                    bool overWrite, bool bg, bool draw) {
  if (!state.currentlyLoading) {
    // checking to make sure the image is in bounds:
    if (x_p + image->width > ILI9488_WIDTH_PX ||
        y_p + image->height > ILI9488_HEIGHT_PX) {
      return HAL_ERROR;
    }

    state.currentlyLoading = true;

    // all pixels in image including ones that are clipped off by the edge of
    // copying state variables for compiler optimization (pointer aliasing)
    uint16_t col_p = 0;                                       // in pixels
    uint32_t pos_p = (ILI9488_WIDTH_PX * y_p) + x_p;          // in pixels
    const uint16_t imgWidth_p = image->width;                 // in pixels
    const uint16_t imgHeight_p = image->height;               // in pixels
    const uint16_t rowSkip_p = ILI9488_WIDTH_PX - imgWidth_p; // in pixels
    const uint8_t *imgData = image->data;
    uint8_t *screenData = state.screenCopy;

    // pre loading background if OR mode is enabled
    if (!overWrite && bg) {
      bgPreload(state.backgroundImage->data, screenData, pos_p, imgWidth_p,
                imgHeight_p);
    }

    // drawing image
    for (uint32_t i = 0; i < image->size; i++) {

      // faster loading algorithm
      bool isOn = i % 2;
      uint8_t remaining_p = imgData[i]; // in pixels

      while (remaining_p > 0) {
        // Sets the current chunk to the largest possible contiguous segment of
        // pixels in the current row
        uint16_t chunk_p = imgWidth_p - col_p;

        // if chunk is more than the remaining pixels needed
        chunk_p = remaining_p > chunk_p ? chunk_p : remaining_p;
        remaining_p -= chunk_p;

        fillScreen(screenData, &pos_p, chunk_p, isOn, overWrite);

        // should only ever == imgWidth as per the logic above
        if ((col_p += chunk_p) >= imgWidth_p) {
          col_p = 0;
          pos_p += rowSkip_p;
        }
      }
    }

    // setting state variables
    state.x = x_p;
    state.y = y_p;
    state.width = imgWidth_p;
    state.height = imgHeight_p;
    state.imageSize = imgWidth_p * imgHeight_p;

    state.currentlyLoading = false;

    if (draw) {
      return ILI9488_Draw(spi);
    }

    return HAL_OK;
  } else {
    return HAL_BUSY;
  }
}

// loads text with transparent background on OR mode
HAL_StatusTypeDef
ILI9488_LoadText(SPI_HandleTypeDef *spi, uint16_t x_p, uint16_t y_p,
                 uint8_t text[], uint8_t textSize, const Character_t *font,
                 const uint8_t fontCount, // number of characters in font
                 const uint8_t charWidth_p, const uint16_t charHeight_p,
                 bool overWrite, bool bg, bool draw) {

  if (!state.currentlyLoading) {
    // checking to make sure the text is in bounds
    if (y_p + charHeight_p > ILI9488_HEIGHT_PX ||
        x_p + charWidth_p > ILI9488_WIDTH_PX) {
      return HAL_ERROR;
    }

    state.currentlyLoading = true;

    uint16_t boundsWidth_p = charWidth_p * textSize; // in pixels

    // clamping font length
    if (x_p + boundsWidth_p > ILI9488_WIDTH_PX) {
      textSize = (ILI9488_WIDTH_PX - x_p) / charWidth_p;
      boundsWidth_p = charWidth_p * textSize;
    }

    const uint32_t bytesPerChar_b =
        (charWidth_p * charHeight_p) >> 3; // in bytes

    const uint8_t charWidth_b = charWidth_p >> 3; // in bytes
    const uint16_t rowSkip_b = ILI9488_WIDTH_BYTES - charWidth_b;

    uint8_t *screenData = state.screenCopy;

    // pre loading background if OR mode is enabled
    if (!overWrite && bg) {
      bgPreload(state.backgroundImage->data, screenData,
                (y_p * ILI9488_WIDTH_PX) + x_p, boundsWidth_p, charHeight_p);
    }

    // iterating through every inputted character
    for (uint16_t charIdx = 0; charIdx < textSize; charIdx++) {
      // loading character
      uint16_t col_b = 0; // in bytes
      uint32_t pos_b = (ILI9488_WIDTH_BYTES * y_p) + (x_p >> 3) +
                       (charIdx * charWidth_b); // in bytes

      // defining current character by using the character array with an ascii
      // offset
      const uint8_t *currentCharacter =
          // if inputted character is out of range for the inputted font, set to
          // a placeholder character
          (text[charIdx] < 32 || (size_t)(text[charIdx] - 32) >= fontCount)
              ? font[0].data
              : font[text[charIdx] - 32].data;

      // loading one byte at a time. This can be done easily as the screen width
      // is a multiple of 8, the x coordinate is a multiple of 8, and the font
      // width is a multiple of 8
      for (uint32_t byte = 0; byte < bytesPerChar_b; byte++) {
        // if the current byte is in bounds of the screen
        if (overWrite) {
          // overwrite mode
          screenData[pos_b] = currentCharacter[byte];
        } else {
          // or mode
          screenData[pos_b] |= currentCharacter[byte];
        }

        pos_b++;
        // incrementing column and row
        if (++col_b >= charWidth_b) {
          col_b = 0;
          pos_b += rowSkip_b;
        }
      }
    }

    // setting state variables
    state.x = x_p;
    state.y = y_p;
    state.width = boundsWidth_p;
    state.height = charHeight_p;
    state.imageSize = boundsWidth_p * charHeight_p;

    state.currentlyLoading = false;

    if (draw) {
      return ILI9488_Draw(spi);
    }

    return HAL_OK;
  } else {
    return HAL_BUSY;
  }
}

// fast refreshing using chunking
HAL_StatusTypeDef ILI9488_Refresh(SPI_HandleTypeDef *spi) {
  // checking if the display is already being modified
  if (!state.currentlyDrawing) {
    // setting status to busy

    state.currentlyDrawing = true;

    HAL_TRY(ILI9488_SetRange(spi, 0, ILI9488_WIDTH_PX - 1, 0,
                             ILI9488_HEIGHT_PX - 1));

    // write data command
    HAL_TRY(ILI9488_Cmd(spi, 0x2C));

    // setting to data mode
    HAL_GPIO_WritePin(DISPLAY_DC_GPIO_Port, DISPLAY_DC_Pin, GPIO_PIN_SET);

    // selecting spi device
    ILI9488_Select();

    state.activeBuf = 0;
    state.imageProgress = 0;

    // setting state to reuse the other callback
    state.imageSize = ILI9488_WIDTH_PX * ILI9488_HEIGHT_PX;
    state.imageTarget = state.imageSize / 2;
    state.width = ILI9488_WIDTH_PX >> 3;
    state.height = ILI9488_HEIGHT_PX;
    state.x = 0;
    state.y = 0;

    state.fillPos = 0;
    state.fillCol = 0;
    state.rowSkip = 0; // full-width refresh, rows are contiguous

    state.activeBuf = 0;
    expandToChunk(state.screenCopy, (uint32_t *)state.buf[state.activeBuf],
                  CHUNK >> 2);

    state.imageProgress += CHUNK;
    state.activeBuf = !state.activeBuf;
    expandToChunk(state.screenCopy, (uint32_t *)state.buf[state.activeBuf],
                  CHUNK >> 2);

    HAL_TRY(HAL_SPI_Transmit_DMA(spi, state.buf[!state.activeBuf], CHUNK));

    return HAL_OK;
  } else {
    return HAL_BUSY;
  }
}

// x and y should be multiples of 8
// draws last loaded image to screen
HAL_StatusTypeDef ILI9488_Draw(SPI_HandleTypeDef *spi) {
  if (!state.currentlyDrawing) {

    // setting status to busy
    state.currentlyDrawing = true;

    // setting fill range to only include the last written screen update
    HAL_TRY(ILI9488_SetRange(spi, state.x, state.x + state.width - 1, state.y,
                             state.y + state.height - 1));

    // conditions should not ever trigger, but still here just in case
    if (state.x + state.width > ILI9488_WIDTH_PX) {
      state.width = ILI9488_WIDTH_PX - state.x;
    }

    if (state.y + state.height > ILI9488_HEIGHT_PX) {
      state.height = ILI9488_HEIGHT_PX - state.y;
    }

    // converting pixels to bytes
    state.x /= 8;
    state.width /= 8;

    // write data command
    HAL_TRY(ILI9488_Cmd(spi, 0x2C));

    // setting to data mode
    HAL_GPIO_WritePin(DISPLAY_DC_GPIO_Port, DISPLAY_DC_Pin, GPIO_PIN_SET);

    // selecting spi device
    ILI9488_Select();

    // double buffering

    // sending image data. chunking data for DMA and memory saving purposes
    state.imageProgress = 0;

    state.imageTarget = state.imageSize / 2;

    state.activeBuf = 0;

    state.fillPos = (uint32_t)ILI9488_WIDTH_BYTES * state.y + state.x;
    state.fillCol = 0;
    state.rowSkip = ILI9488_WIDTH_BYTES - state.width;

    if (state.imageTarget <= CHUNK) {
      expandToChunk(state.screenCopy, (uint32_t *)state.buf[state.activeBuf],
                    state.imageTarget >> 2);
      HAL_TRY(HAL_SPI_Transmit_DMA(spi, state.buf[state.activeBuf],
                                   state.imageTarget));
      state.imageProgress = state.imageTarget;
    } else {
      expandToChunk(state.screenCopy, (uint32_t *)state.buf[state.activeBuf],
                    CHUNK >> 2);
      state.imageProgress += CHUNK;
      state.activeBuf = !state.activeBuf;

      uint32_t remaining = state.imageTarget - state.imageProgress;
      expandToChunk(state.screenCopy, (uint32_t *)state.buf[state.activeBuf],
                    (remaining < CHUNK ? remaining : CHUNK) >> 2);
      HAL_TRY(HAL_SPI_Transmit_DMA(spi, state.buf[!state.activeBuf], CHUNK));
    }

    return HAL_OK;
  } else {
    return HAL_BUSY;
  }
}

// callback that is called when HAL_SPI_Transmit_DMA finishes
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
  // checking if it's for the current serial peripheral
  if (hspi->Instance == SPI1) {

    // done drawing condition
    if (state.imageProgress >= state.imageTarget) {
      ILI9488_Deselect();
      state.currentlyDrawing = 0;
      return;
    }

    // partial chunk remaining
    else if (state.imageTarget - state.imageProgress < CHUNK) {
      // can't do anything about error handling here?
      // maybe TODO find a way to re run the draw function if error occurs here
      HAL_SPI_Transmit_DMA(hspi, state.buf[state.activeBuf],
                           state.imageTarget - state.imageProgress);
      // set progress to finished
      state.imageProgress += CHUNK;
    }

    // full chunk remaining
    else {
      // incrementing image progress
      state.imageProgress += CHUNK;
      // transmitting loaded buffer
      HAL_SPI_Transmit_DMA(hspi, state.buf[state.activeBuf], CHUNK);
      // toggling active buffer
      state.activeBuf = !state.activeBuf;

      expandToChunk(state.screenCopy, (uint32_t *)state.buf[state.activeBuf],
                    CHUNK >> 2);
    }
  }
}
