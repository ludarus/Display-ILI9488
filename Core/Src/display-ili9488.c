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
#include "File_006_ObjNum_005_480x320_6_18_26_C.h"
#include "main.h"
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

// replaces sections of an expanded background with image coloured pixels
static inline void fillExpanded(uint8_t *buffer, uint32_t *position,
                                uint16_t count, const uint8_t onColour,
                                bool isOn) {
  // position is in pixels, should be mod the size of buffer
  if (isOn) {
    // filling up leading pixel
    // if the current position isn't byte aligned
    if ((*position) % 2 && count > 0) {
      uint8_t mask = onColour >> 4;
      // clearing the second pixel of the byte
      buffer[((*position) / 2) % CHUNK] &= 0b11110000;
      // setting the second pixel
      buffer[((*position) / 2) % CHUNK] |= mask;

      count--;
      (*position)++;
    }

    // filling up middle pixels
    for (uint8_t byte = 0; byte < count / 2; byte++) {
      buffer[((*position) / 2) % CHUNK] = onColour;
      (*position) += 2;
    }

    // filling up trailing pixel
    if (count % 2) {
      uint8_t mask = onColour << 4;
      // clearing the first pixel of the byte
      buffer[((*position) / 2) % CHUNK] &= 0b00001111;
      // setting the second pixel
      buffer[((*position) / 2) % CHUNK] |= mask;
      (*position)++;
    }

  } else {
    (*position) += count;
    return;
  }
}

// expands a bitpacked byte to the proper data format & colour resolution to
// transmit to the display.
// Takes 1 byte which contains 8 pixels worth of information and expands it into
// 4 bytes of colour coded data
static inline void expandBgToChunk(uint8_t *background, uint32_t *dst,
                                   const uint32_t wordCount,
                                   const uint16_t rowSkip, uint32_t *pos,
                                   uint16_t *col, const uint16_t width) {

  for (uint32_t i = 0; i < wordCount; i++) {
    dst[i] = bgPixelTable[background[(*pos)]];
    (*pos)++;
    if (++(*col) == width) {
      (*col) = 0;
      (*pos) += rowSkip;
    }
  }

  state.fillBgPos = (*pos);
  state.fillBgCol = (*col);
}

static inline void expandImgToChunk(Image_t *img, const uint8_t colour,
                                    uint8_t *buf, const uint32_t wordCount) {

  uint32_t pos_p = state.fillImgPos;
  const uint32_t target = pos_p + (wordCount << 3);

  // filling buffer up
  const uint8_t *imgData = img->data;
  uint32_t imgRem_p = state.fillImgRem;
  uint32_t imgIdx = state.fillImgIdx;

  // walking through the rle data
  while (pos_p < target) {
    uint32_t chunk = imgRem_p < (target - pos_p) ? imgRem_p : (target - pos_p);
    fillExpanded(buf, &pos_p, chunk, colour, imgIdx % 2);
    rleAdvance(imgData, &imgRem_p, &imgIdx, chunk);
  }

  state.fillImgPos = pos_p;
  state.fillImgRem = imgRem_p;
  state.fillImgIdx = imgIdx;
}

// helper function to get the correct mask from the nibble table
static inline uint32_t fontMask(uint8_t glyphByte) {
  return (uint32_t)nibbleTable[glyphByte & 0x0F] |
         ((uint32_t)nibbleTable[glyphByte >> 4] << 16);
}

static inline void expandTextToChunk(const uint8_t colour, uint32_t *dst,
                                     const uint32_t wordCount) {

  const uint32_t expandedColour = (uint32_t)(colour) | (uint32_t)(colour << 8) |
                                  (uint32_t)(colour << 16) |
                                  (uint32_t)(colour << 24);

  for (uint32_t i = 0; i < wordCount; i++) {
    // masking and setting the on pixels to the correct colour
    uint32_t mask = fontMask(state.font[state.text[state.currentChar] - 32]
                                 .data[state.textPos_b + state.textCol_b]);
    dst[i] = (dst[i] & ~mask) | (expandedColour & mask);

    // looping col cursor because it's the same y coordinate no matter which
    // char

    if (++state.textCol_b >= state.charWidth_b) {
      state.textCol_b = 0;
      state.currentChar++;
      if (state.currentChar >= state.textSize) {
        state.textPos_b += state.charWidth_b;
        state.currentChar = 0;
      }
    }
  }
}

// bitpacks a contiguous section of a buffer while advancing position pointer
static inline void fillBitpacked(uint8_t *buffer, uint32_t *position,
                                 uint16_t count, const bool isOn,
                                 const bool overWrite) {
  // filling leading bits
  // if the current position isn't byte aligned
  if ((*position) % 8 != 0) {
    uint8_t offset = (*position) % 8;
    uint8_t leading = 8 - offset;
    leading = leading > count ? count : leading;
    uint8_t mask = (uint8_t)(((1u << leading) - 1u) << offset);

    // if the pixels to write are on or off, mask accordingly
    if (isOn) {
      buffer[(*position) >> 3] |= mask;
    } else if (overWrite) {
      buffer[(*position) >> 3] &= (uint8_t)~mask;
    }

    // decrementing count and incrementing position
    count -= leading;
    (*position) += leading;
  }

  // filling middle bytes
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

  // filling trailing bits
  uint8_t trailing = count % 8;
  // check for unaligned bits
  if (trailing != 0) {
    if (isOn) {
      buffer[(*position) >> 3] |= 0xFF >> (8 - trailing);
    } else if (overWrite) {
      buffer[(*position) >> 3] &= 0xFF << trailing;
    }
    (*position) += trailing;
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

HAL_StatusTypeDef ILI9488_SetBackground_Col(SPI_HandleTypeDef *spi,
                                            const Image_t *bg) {

  // making sure requested image is full screen
  if (bg->height != 320 || bg->width != 480) {
    return HAL_ERROR;
  }

  // filling buffer up
  const uint8_t *bgData = bg->data;
  uint32_t bgRem_p = bgData[0];
  uint32_t bgIdx = 0;
  uint32_t position_p = 0;

  // looping
  while (position_p < (ILI9488_WIDTH_PX * ILI9488_HEIGHT_PX) - 1) {
    // filling up the buffer
    fillBitpacked(state.background, &position_p, bgRem_p, bgIdx % 2, true);
    // walking through the rle data
    rleAdvance(bgData, &bgRem_p, &bgIdx, bgRem_p);
  }

  // drawing the background
  ILI9488_BlitBackground(spi);

  return HAL_OK;
}

// initializes the ILI9488
HAL_StatusTypeDef ILI9488_Init(SPI_HandleTypeDef *spi,
                               TIM_HandleTypeDef *backlightTimer) {

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
  //  HAL_GPIO_WritePin(SWITCH_POWER_GPIO_Port, SWITCH_POWER_Pin, GPIO_PIN_SET);

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

  // initializing background to empty image
  ILI9488_SetBackground_Col(spi,
                            (Image_t *)&File_006_ObjNum_005_480x320_6_18_26_C);

  return HAL_OK;
}

// temp
HAL_StatusTypeDef ILI9488_SetBackground_Mono(const Image_t *bg) {
  return HAL_OK;
}
HAL_StatusTypeDef ILI9488_Refresh_Mono(SPI_HandleTypeDef *spi) {
  return HAL_OK;
}
HAL_StatusTypeDef ILI9488_LoadImage_Mono(SPI_HandleTypeDef *spi, uint16_t x_p,
                                         uint16_t y_p, const Image_t *image,
                                         bool overWrite, bool bg, bool draw) {
  return HAL_OK;
}
HAL_StatusTypeDef
ILI9488_LoadText_Mono(SPI_HandleTypeDef *spi, uint16_t x_p, uint16_t y_p,
                      uint8_t text[], uint8_t textSize, const Character_t *font,
                      const uint8_t fontCount, // number of characters in font
                      const uint8_t charWidth_p, const uint16_t charHeight_p,
                      bool overWrite, bool bg, bool draw) {
  return HAL_OK;
}

// x and y should be multiples of 8
// draws background to screen
HAL_StatusTypeDef ILI9488_BlitBackground(SPI_HandleTypeDef *spi) {

  if (state.drawStatus == DS_NONE) {

    // setting status to busy
    state.drawStatus = DS_BG;

    // converting pixels to bytes
    state.width = ILI9488_WIDTH_BYTES;

    // sending image data. chunking data for DMA and memory saving purposes
    state.progress = 0;
    state.target = (ILI9488_WIDTH_PX * ILI9488_HEIGHT_PX) / 2;
    state.activeBuf = 0;
    state.fillBgPos = 0;
    state.fillBgCol = 0;
    state.rowSkipBg = 0;

    // setting fill range to only include the last written screen update
    HAL_TRY(ILI9488_SetRange(spi, 0, ILI9488_WIDTH_PX - 1, 0,
                             ILI9488_HEIGHT_PX - 1));
    // write data command
    HAL_TRY(ILI9488_Cmd(spi, 0x2C));

    // setting to data mode
    HAL_GPIO_WritePin(DISPLAY_DC_GPIO_Port, DISPLAY_DC_Pin, GPIO_PIN_SET);

    // selecting spi device
    ILI9488_Select();

    // double buffering

    // filling up first chunk
    if (state.target <= CHUNK) {
      expandBgToChunk(state.background, (uint32_t *)state.buf[state.activeBuf],
                      state.target >> 2, state.rowSkipBg, &state.fillBgPos,
                      &state.fillBgCol, state.width);
      HAL_TRY(
          HAL_SPI_Transmit_DMA(spi, state.buf[state.activeBuf], state.target));
      state.progress = state.target;
    } else {
      expandBgToChunk(state.background, (uint32_t *)state.buf[state.activeBuf],
                      CHUNK >> 2, state.rowSkipBg, &state.fillBgPos,
                      &state.fillBgCol, state.width);
      state.progress += CHUNK;
      state.activeBuf = !state.activeBuf;

      // filling up second chunk
      uint32_t remaining = state.target - state.progress;
      expandBgToChunk(state.background, (uint32_t *)state.buf[state.activeBuf],
                      (remaining < CHUNK ? remaining : CHUNK) >> 2,
                      state.rowSkipBg, &state.fillBgPos, &state.fillBgCol,
                      state.width);
      HAL_TRY(HAL_SPI_Transmit_DMA(spi, state.buf[!state.activeBuf], CHUNK));
    }

    return HAL_OK;
  } else {
    return HAL_BUSY;
  }
}

HAL_StatusTypeDef ILI9488_BlitImage(SPI_HandleTypeDef *spi, uint16_t x_p,
                                    uint16_t y_p, const Image_t *image,
                                    const uint8_t colour) {
  if (state.drawStatus == DS_NONE) {
    // checking to make sure the image is in bounds:
    if (x_p + image->width > ILI9488_WIDTH_PX ||
        y_p + image->height > ILI9488_HEIGHT_PX) {
      return HAL_ERROR;
    }

    state.drawStatus = DS_IMG;

    // setting x y and width and height
    state.width = image->width / 8;

    state.activeBuf = 0;
    state.progress = 0;
    state.target = (image->width * image->height) / 2;

    state.fillBgPos = (uint32_t)ILI9488_WIDTH_BYTES * y_p + (x_p / 8);
    state.fillBgCol = 0;
    state.rowSkipBg = ILI9488_WIDTH_BYTES - state.width;
    state.image = (Image_t *)image;
    state.colour = colour;

    state.fillImgPos = 0;
    state.fillImgIdx = 0;
    state.fillImgRem = image->data[0];

    // setting range
    HAL_TRY(ILI9488_SetRange(spi, x_p, x_p + image->width - 1, y_p,
                             y_p + image->height - 1));
    // write data command
    HAL_TRY(ILI9488_Cmd(spi, 0x2C));

    // setting to data mode
    HAL_GPIO_WritePin(DISPLAY_DC_GPIO_Port, DISPLAY_DC_Pin, GPIO_PIN_SET);

    // selecting spi device
    ILI9488_Select();

    // filling up first chunk
    if (state.target <= CHUNK) {
      expandBgToChunk(state.background, (uint32_t *)state.buf[state.activeBuf],
                      state.target >> 2, state.rowSkipBg, &state.fillBgPos,
                      &state.fillBgCol, state.width);
      // loading image over background
      expandImgToChunk((Image_t *)image, colour, state.buf[state.activeBuf],
                       CHUNK / 4);
      HAL_TRY(
          HAL_SPI_Transmit_DMA(spi, state.buf[state.activeBuf], state.target));
      state.progress = state.target;
    } else {
      expandBgToChunk(state.background, (uint32_t *)state.buf[state.activeBuf],
                      CHUNK >> 2, state.rowSkipBg, &state.fillBgPos,
                      &state.fillBgCol, state.width);
      // loading image over background
      expandImgToChunk((Image_t *)image, colour, state.buf[state.activeBuf],
                       CHUNK / 4);
      state.progress += CHUNK;
      state.activeBuf = !state.activeBuf;

      // filling up second chunk
      uint32_t remaining = state.target - state.progress;
      expandBgToChunk(state.background, (uint32_t *)state.buf[state.activeBuf],
                      (remaining < CHUNK ? remaining : CHUNK) >> 2,
                      state.rowSkipBg, &state.fillBgPos, &state.fillBgCol,
                      state.width);
      // loading image over background
      expandImgToChunk((Image_t *)image, colour, state.buf[state.activeBuf],
                       (remaining < CHUNK ? remaining : CHUNK) >> 2);
      HAL_TRY(HAL_SPI_Transmit_DMA(spi, state.buf[!state.activeBuf], CHUNK));
    }
    return HAL_OK;
  } else {
    return HAL_BUSY;
  }
}

HAL_StatusTypeDef
ILI9488_BlitText(SPI_HandleTypeDef *spi, uint16_t x_p, uint16_t y_p,
                 uint8_t text[], const uint16_t textSize,
                 const Character_t *font, const uint8_t fontCount,
                 const uint8_t charWidth_p, const uint8_t charHeight_p,
                 const uint8_t colour) {
  if (state.drawStatus == DS_NONE) {
    // checking to make sure the text is in bounds
    if (y_p + charHeight_p > ILI9488_HEIGHT_PX ||
        x_p + charWidth_p > ILI9488_WIDTH_PX) {
      return HAL_ERROR;
    }

    // text processing to make sure all characters are displayable
    for (uint8_t i = 0; i < textSize; i++) {
      // if not displayable set to blank character
      if (text[i] < 32 || text[i] >= fontCount + 32) {
        text[i] = 32;
      }
    }

    state.drawStatus = DS_TEXT;

    uint16_t boundsWidth_p = charWidth_p * textSize; // in pixels

    // setting x y and width and height
    state.width = boundsWidth_p / 8;

    state.activeBuf = 0;
    state.progress = 0;
    state.target = (boundsWidth_p * charHeight_p) / 2;

    state.fillBgPos = (uint32_t)ILI9488_WIDTH_BYTES * y_p + (x_p / 8);
    state.fillBgCol = 0;
    state.rowSkipBg = ILI9488_WIDTH_BYTES - state.width;

    state.text = text;
    state.textSize = textSize;
    state.colour = colour;
    state.currentChar = 0;
    state.textPos_b = 0;
    state.textCol_b = 0;
    state.font = font;
    state.charWidth_b = charWidth_p / 8;

    // setting range
    HAL_TRY(ILI9488_SetRange(spi, x_p, x_p + boundsWidth_p - 1, y_p,
                             y_p + charHeight_p - 1));

    // write data command
    HAL_TRY(ILI9488_Cmd(spi, 0x2C));

    // setting to data mode
    HAL_GPIO_WritePin(DISPLAY_DC_GPIO_Port, DISPLAY_DC_Pin, GPIO_PIN_SET);

    // selecting spi device
    ILI9488_Select();

    // filling up first chunk
    if (state.target <= CHUNK) {
      expandBgToChunk(state.background, (uint32_t *)state.buf[state.activeBuf],
                      state.target >> 2, state.rowSkipBg, &state.fillBgPos,
                      &state.fillBgCol, state.width);
      expandTextToChunk(state.colour, (uint32_t *)state.buf[state.activeBuf],
                        CHUNK / 4);

      HAL_TRY(
          HAL_SPI_Transmit_DMA(spi, state.buf[state.activeBuf], state.target));

      state.progress = state.target;
    } else {
      expandBgToChunk(state.background, (uint32_t *)state.buf[state.activeBuf],
                      CHUNK >> 2, state.rowSkipBg, &state.fillBgPos,
                      &state.fillBgCol, state.width);
      expandTextToChunk(state.colour, (uint32_t *)state.buf[state.activeBuf],
                        CHUNK / 4);

      state.progress += CHUNK;
      state.activeBuf = !state.activeBuf;

      // filling up second chunk
      uint32_t remaining = state.target - state.progress;
      expandBgToChunk(state.background, (uint32_t *)state.buf[state.activeBuf],
                      (remaining < CHUNK ? remaining : CHUNK) >> 2,
                      state.rowSkipBg, &state.fillBgPos, &state.fillBgCol,
                      state.width);
      expandTextToChunk(state.colour, (uint32_t *)state.buf[state.activeBuf],
                        CHUNK / 4);

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
    if (state.progress >= state.target) {
      ILI9488_Deselect();
      state.drawStatus = DS_NONE;
      return;
    }

    // partial chunk remaining
    else if (state.target - state.progress < CHUNK) {

      HAL_SPI_Transmit_DMA(hspi, state.buf[state.activeBuf],
                           state.target - state.progress);
      // set progress to finished
      state.progress += CHUNK;
    }

    // full chunk remaining
    else {
      // incrementing image progress
      state.progress += CHUNK;
      // transmitting loaded buffer
      HAL_SPI_Transmit_DMA(hspi, state.buf[state.activeBuf], CHUNK);
      // toggling active buffer
      state.activeBuf = !state.activeBuf;

      switch (state.drawStatus) {
      case DS_BG:
        expandBgToChunk(
            state.background, (uint32_t *)state.buf[state.activeBuf], CHUNK / 4,
            state.rowSkipBg, &state.fillBgPos, &state.fillBgCol, state.width);
        break;
      case DS_IMG:
        expandBgToChunk(
            state.background, (uint32_t *)state.buf[state.activeBuf], CHUNK / 4,
            state.rowSkipBg, &state.fillBgPos, &state.fillBgCol, state.width);
        expandImgToChunk(state.image, state.colour, state.buf[state.activeBuf],
                         CHUNK / 4);
        break;
      case DS_TEXT:
        expandBgToChunk(
            state.background, (uint32_t *)state.buf[state.activeBuf], CHUNK / 4,
            state.rowSkipBg, &state.fillBgPos, &state.fillBgCol, state.width);
        expandTextToChunk(state.colour, (uint32_t *)state.buf[state.activeBuf],
                          CHUNK / 4);
        break;
      }
    }
  }
}
