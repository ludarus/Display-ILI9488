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
#include "character.h"
#include "font.h"
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
static inline void rleAdvance(const uint8_t *data, uint8_t *remaining_p,
                              uint32_t *index, uint32_t count_p) {
  // local clone for pointer aliasing
  uint32_t idx = (*index);
  uint8_t rem = (*remaining_p);

  // traversing RLE stream to get to targetted value
  while (count_p > 0 || rem == 0) {
    if (rem > count_p) {
      rem -= count_p;
      (*index) = idx;
      (*remaining_p) = rem;
      return;
    }
    count_p -= rem;

    idx++;
    rem = data[idx];
  }

  (*index) = idx;
  (*remaining_p) = rem;
}

// replaces sections of an expanded background with image coloured pixels
static inline void fillExpanded(uint8_t *buf, uint32_t *pos_p, uint16_t count_p,
                                const uint8_t onColour, bool isOn,
                                bool overWrite) {
  uint32_t pos = (*pos_p);

  // position is in pixels, should be mod the size of buffer
  if (isOn || overWrite) {
    // filling up leading pixel
    // if the current position isn't byte aligned
    if (pos % 2 && count_p > 0) {
      uint8_t mask = (isOn) ? (onColour << 5) >> 5 : 0;
      // clearing the second pixel of the byte
      buf[(pos / 2) % CHUNK] &= 0b00111000;
      // setting the second pixel
      buf[(pos / 2) % CHUNK] |= mask;

      count_p--;
      pos++;
    }

    // filling up middle pixels
    for (uint8_t byte = 0; byte < count_p / 2; byte++) {
      buf[(pos / 2) % CHUNK] = (isOn) ? onColour : 0;
      pos += 2;
    }

    // filling up trailing pixel
    if (count_p % 2) {
      uint8_t mask = isOn ? (onColour >> 3) << 3 : 0;
      // clearing the first pixel of the byte
      buf[(pos / 2) % CHUNK] &= 0b00000111;
      // setting the second pixel
      buf[(pos / 2) % CHUNK] |= mask;
      pos++;
    }

  } else {
    pos += count_p;
  }

  (*pos_p) = pos;
}

// expands a bitpacked byte to the proper data format & colour resolution to
// transmit to the display.
// Takes 1 byte which contains 8 pixels worth of information and expands it into
// 4 bytes of colour coded data
static inline void expandBgToChunk(uint8_t *background, uint32_t *buf_b,
                                   const uint32_t count_p,
                                   const uint16_t rowSkip_b, uint32_t *pos_b,
                                   uint16_t *col_b, const uint16_t width_b) {
  uint32_t pos = (*pos_b);
  uint16_t col = (*col_b);

  for (uint32_t i = 0; i < count_p; i++) {
    buf_b[i] = bgPixelTable[background[pos]];
    pos++;
    if (++col == width_b) {
      col = 0;
      pos += rowSkip_b;
    }
  }

  (*pos_b) = pos;
  (*col_b) = col;
}

static inline void expandImgToChunk(Image_t *img, const uint8_t colour,
                                    uint8_t *buf_p, const uint32_t count_p,
                                    uint32_t *pos_p, uint16_t *rem_p,
                                    uint32_t *index, const bool overWrite) {

  uint32_t pos = (*pos_p);
  // rem_p should always be 8 bit
  uint8_t rem = (uint8_t)(*rem_p);
  uint32_t idx = (*index);

  const uint32_t target = pos + (count_p << 3);

  // filling buffer up
  const uint8_t *imgData = img->data;

  // walking through the rle data
  while (pos < target) {
    uint32_t chunk = rem < (target - pos) ? rem : (target - pos);
    fillExpanded(buf_p, &pos, chunk, colour, idx % 2, overWrite);
    rleAdvance(imgData, &rem, &idx, chunk);
  }

  (*pos_p) = pos;
  (*rem_p) = rem;
  (*index) = idx;
}

// helper function to get the correct mask from the nibble table
static inline uint32_t fontMask(uint8_t glyphByte) {
  return (uint32_t)nibbleTable[glyphByte & 0x0F] |
         ((uint32_t)nibbleTable[glyphByte >> 4] << 16);
}

static inline void expandTextToChunk(const uint32_t expandedColour,
                                     uint32_t *buf_b, const uint32_t count_p,
                                     const Character_t *font, uint8_t *text,
                                     uint32_t *currentChar, uint32_t *pos_b,
                                     uint16_t *col_b, const uint8_t charWidth_b,
                                     const uint8_t textSize,
                                     const bool overWrite) {

  uint32_t cChar = *currentChar;
  uint32_t pos = *pos_b;
  uint16_t col = *col_b;

  for (uint32_t i = 0; i < count_p; i++) {
    uint32_t mask = fontMask(font[text[cChar] - 32].data[pos + col]);
    if (overWrite) {
      buf_b[i] = mask & expandedColour;
    } else {
      buf_b[i] = (buf_b[i] & ~mask) | (expandedColour & mask);
    }

    if (++col >= charWidth_b) {
      col = 0;
      cChar++;
      if (cChar >= textSize) {
        pos += charWidth_b;
        cChar = 0;
      }
    }
  }

  *currentChar = cChar;
  *pos_b = pos;
  *col_b = col;
}

// bitpacks a contiguous section of a buffer while advancing position pointer
static inline void fillBitpacked(uint8_t *buf_p, uint32_t *pos_p,
                                 uint16_t count_p, const bool isOn,
                                 const bool overWrite) {
  uint32_t pos = (*pos_p);

  // filling leading bits
  // if the current position isn't byte aligned
  if (pos % 8 != 0) {
    uint8_t offset = pos % 8;
    uint8_t leading = 8 - offset;
    leading = leading > count_p ? count_p : leading;
    uint8_t mask = (uint8_t)(((1u << leading) - 1u) << offset);

    // if the pixels to write are on or off, mask accordingly
    if (isOn) {
      buf_p[pos >> 3] |= mask;
    } else if (overWrite) {
      buf_p[pos >> 3] &= (uint8_t)~mask;
    }

    // decrementing count and incrementing position
    count_p -= leading;
    pos += leading;
  }

  // filling middle bytes
  for (uint8_t byte = 0; byte < count_p >> 3; byte++) {
    if (isOn) {
      // write byte
      buf_p[pos >> 3] = 0xFF;
    } else if (overWrite) {
      // clear byte
      buf_p[pos >> 3] = 0;
    }

    // incrementing global position
    pos += 8;
  }

  // filling trailing bits
  uint8_t trailing = count_p % 8;
  // check for unaligned bits
  if (trailing != 0) {
    if (isOn) {
      buf_p[pos >> 3] |= 0xFF >> (8 - trailing);
    } else if (overWrite) {
      buf_p[pos >> 3] &= 0xFF << trailing;
    }
    pos += trailing;
  }

  (*pos_p) = pos;
}

// private display functions

// LCD hardware Reset, active low
void ILI9488_Reset(void) {
  // setting the reset pin to low to signal a reset
  HAL_GPIO_WritePin(DISPLAY_RESET_GPIO_Port, DISPLAY_RESET_Pin, GPIO_PIN_RESET);
  HAL_Delay(10);
  // setting the pin to high (default state)
  HAL_GPIO_WritePin(DISPLAY_RESET_GPIO_Port, DISPLAY_RESET_Pin, GPIO_PIN_SET);
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

// debugging function
void DEBUG_sendpixels(SPI_HandleTypeDef *spi, uint8_t pixel, uint32_t count) {

  ILI9488_Cmd(spi, 0x2C);

  // setting to data mode
  HAL_GPIO_WritePin(DISPLAY_DC_GPIO_Port, DISPLAY_DC_Pin, GPIO_PIN_SET);

  ILI9488_Select();

  for (uint32_t i = 0; i < count; i++) {
    HAL_SPI_Transmit(spi, &pixel, 1, HAL_MAX_DELAY);
  }

  ILI9488_Deselect();
}

// utility function to set the writing range of the controller
HAL_StatusTypeDef ILI9488_SetRange(SPI_HandleTypeDef *spi, uint16_t colStart,
                                   uint16_t colEnd, uint16_t rowStart,
                                   uint16_t rowEnd) {

  // no-op command to cancel any previous write
  ILI9488_Cmd(spi, 0);

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
  uint8_t paset[] = {

      (uint8_t)(rowStart >> 8),

      (uint8_t)(rowStart & 0xFF),

      (uint8_t)(rowEnd >> 8),

      (uint8_t)(rowEnd & 0xFF)

  };

  HAL_TRY(ILI9488_Data(spi, paset, 4));

  return HAL_OK;
}

// draws background to screen
HAL_StatusTypeDef ILI9488_BlitBackground(SPI_HandleTypeDef *spi) {

  if (state.drawStatus == DS_NONE) {

    // setting status to busy
    state.drawStatus = DS_BG;

    // setting fill range to only include the last written screen update
    HAL_TRY(ILI9488_SetRange(spi, 0, ILI9488_WIDTH_PX - 1, 0,
                             ILI9488_HEIGHT_PX - 1));

    // constants
    state.width_b = ILI9488_WIDTH_BYTES;
	 state.overWrite = false;

    // buffer progress cursor data
    state.progress_eb = 0;
    state.target_eb = (ILI9488_WIDTH_PX * ILI9488_HEIGHT_PX) / 2;
    state.activeBuf = 0;

    // background cursor data
    state.bgPos_b = 0;
    state.bgCol_b = 0;
    state.bgRowSkip_b = 0;

    // write data command
    HAL_TRY(ILI9488_Cmd(spi, 0x2C));

    // setting to data mode
    HAL_GPIO_WritePin(DISPLAY_DC_GPIO_Port, DISPLAY_DC_Pin, GPIO_PIN_SET);

    // selecting spi device
    ILI9488_Select();

    // double buffering

    // filling up first chunk
    if (state.target_eb <= CHUNK) {
      expandBgToChunk(state.background, (uint32_t *)state.buf[state.activeBuf],
                      state.target_eb / 4, state.bgRowSkip_b, &state.bgPos_b,
                      &state.bgCol_b, state.width_b);

      HAL_TRY(HAL_SPI_Transmit_DMA(spi, state.buf[state.activeBuf],
                                   state.target_eb));

      state.progress_eb = state.target_eb;
    } else {
      expandBgToChunk(state.background, (uint32_t *)state.buf[state.activeBuf],
                      PX_PER_CHUNK, state.bgRowSkip_b, &state.bgPos_b,
                      &state.bgCol_b, state.width_b);

      state.progress_eb += CHUNK;
      // inverting the active buffer
      state.activeBuf = !state.activeBuf;

      // filling up second chunk
      uint32_t remaining_p = (state.target_eb - state.progress_eb) / 4;
      uint32_t clampedChunk_p =
          remaining_p < PX_PER_CHUNK ? remaining_p : PX_PER_CHUNK;

      expandBgToChunk(state.background, (uint32_t *)state.buf[state.activeBuf],
                      clampedChunk_p, state.bgRowSkip_b, &state.bgPos_b,
                      &state.bgCol_b, state.width_b);

      HAL_TRY(HAL_SPI_Transmit_DMA(spi, state.buf[!state.activeBuf], CHUNK));
    }

    return HAL_OK;
  } else {
    return HAL_BUSY;
  }
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

HAL_StatusTypeDef ILI9488_SetBackground(SPI_HandleTypeDef *spi,
                                        const Image_t *bg) {

  // making sure requested image is full screen
  if (bg->height != 320 || bg->width != 480) {
    return HAL_ERROR;
  }

  // filling buffer up
  const uint8_t *bgData = bg->data;
  uint8_t bgRem_p = bgData[0];
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

  // hardware reset
  ILI9488_Reset();
  HAL_Delay(100);

  // software reset
  HAL_TRY(ILI9488_Cmd(spi, 0x01));
  HAL_Delay(100);

  // exit sleep mode
  HAL_TRY(ILI9488_Cmd(spi, 0x11));
  HAL_Delay(10);

  // powering testing switches
  //  HAL_GPIO_WritePin(SWITCH_POWER_GPIO_Port, SWITCH_POWER_Pin, GPIO_PIN_SET);

  // backlight on
  // starting display backlight pwm timer
  HAL_TIM_PWM_Start(backlightTimer, TIM_CHANNEL_1);

  // configuring extended command set for spi write
  HAL_TRY(ILI9488_Cmd(spi, 0xF7));

  // uint8_t unlockData[] = {0xA9, 0x51, 0x2C, 0x82};
  // HAL_TRY(ILI9488_Data(spi, &unlockData[0], 4));

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
  // format: 0 0 R G B R G B
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

  // enabling partial mode
  HAL_TRY(ILI9488_Cmd(spi, 0x12));

  // display on
  HAL_TRY(ILI9488_Cmd(spi, 0x29));

  return HAL_OK;
}

HAL_StatusTypeDef ILI9488_BlitImage(SPI_HandleTypeDef *spi, uint16_t x_p,
                                    uint16_t y_p, const Image_t *image,
                                    const uint8_t colour,
                                    const bool overWrite) {
  if (state.drawStatus == DS_NONE) {
    // checking to make sure the image is in bounds:
    if (x_p + image->width > ILI9488_WIDTH_PX ||
        y_p + image->height > ILI9488_HEIGHT_PX) {
      return HAL_ERROR;
    }

    // setting draw status
    state.drawStatus = DS_IMG;

    // setting range
    HAL_TRY(ILI9488_SetRange(spi, x_p, x_p + image->width - 1, y_p,
                             y_p + image->height - 1));

    // constants
    state.width_b = image->width / 8;
    state.image = (Image_t *)image;
    state.colour = (uint32_t)colour;
    state.overWrite = overWrite;

    // buffer progress cursor data
    state.progress_eb = 0;
    state.target_eb = (image->width * image->height) / 2;
    state.activeBuf = 0;

    // background cursor data
    state.bgCol_b = 0;
    state.bgPos_b = (ILI9488_WIDTH_BYTES * y_p) + (x_p / 8);
    state.bgRowSkip_b = ILI9488_WIDTH_BYTES - state.width_b;

    // image cursor data
    state.objPos = 0;
    state.objIdx = 0;
    state.objCount = image->data[0];

    // write data command
    HAL_TRY(ILI9488_Cmd(spi, 0x2C));

    // setting to data mode
    HAL_GPIO_WritePin(DISPLAY_DC_GPIO_Port, DISPLAY_DC_Pin, GPIO_PIN_SET);

    // selecting spi device
    ILI9488_Select();

    // filling up first chunk
    if (state.target_eb <= CHUNK) {
      if (!overWrite) {
        expandBgToChunk(state.background,
                        (uint32_t *)state.buf[state.activeBuf],
                        state.target_eb / 4, state.bgRowSkip_b, &state.bgPos_b,
                        &state.bgCol_b, state.width_b);
      }
      // loading image over background
      expandImgToChunk((Image_t *)image, colour, state.buf[state.activeBuf],
                       state.target_eb / 4, &state.objPos, &state.objCount,
                       &state.objIdx, overWrite);

      HAL_TRY(HAL_SPI_Transmit_DMA(spi, state.buf[state.activeBuf],
                                   state.target_eb));

      state.progress_eb = state.target_eb;

    } else {
      if (!overWrite) {
        expandBgToChunk(state.background,
                        (uint32_t *)state.buf[state.activeBuf], PX_PER_CHUNK,
                        state.bgRowSkip_b, &state.bgPos_b, &state.bgCol_b,
                        state.width_b);
      }
      // loading image over background
      expandImgToChunk((Image_t *)image, colour, state.buf[state.activeBuf],
                       PX_PER_CHUNK, &state.objPos, &state.objCount,
                       &state.objIdx, overWrite);

      state.progress_eb += CHUNK;
      // inverting the active buffer
      state.activeBuf = !state.activeBuf;

      // filling up second chunk
      uint32_t remaining_p = (state.target_eb - state.progress_eb) / 4;
      uint32_t clampedChunk_p =
          remaining_p < PX_PER_CHUNK ? remaining_p : PX_PER_CHUNK;

      if (!overWrite) {
        expandBgToChunk(state.background,
                        (uint32_t *)state.buf[state.activeBuf], clampedChunk_p,
                        state.bgRowSkip_b, &state.bgPos_b, &state.bgCol_b,
                        state.width_b);
      }
      // loading image over background
      expandImgToChunk((Image_t *)image, colour, state.buf[state.activeBuf],
                       clampedChunk_p, &state.objPos, &state.objCount,
                       &state.objIdx, overWrite);
      HAL_TRY(HAL_SPI_Transmit_DMA(spi, state.buf[!state.activeBuf], CHUNK));
    }

    return HAL_OK;
  } else {
    return HAL_BUSY;
  }
}

HAL_StatusTypeDef ILI9488_BlitText(SPI_HandleTypeDef *spi, uint16_t x_p,
                                   uint16_t y_p, uint8_t text[],
                                   const uint16_t textSize,
                                   const uint8_t colour, const bool overWrite) {

  if (state.drawStatus == DS_NONE) {
    // checking to make sure the text is in bounds
    const uint16_t boundsWidth_p = CHARWIDTH * textSize;

    if (y_p + CHARHEIGHT > ILI9488_HEIGHT_PX ||
        x_p + boundsWidth_p > ILI9488_WIDTH_PX) {
      return HAL_ERROR;
    }

    // text processing to make sure all characters are displayable
    for (uint8_t i = 0; i < textSize; i++) {
      // if not displayable set to blank character
      if (text[i] < 32 || text[i] >= FONTSIZE + 32) {
        text[i] = 32;
      }
    }

    // setting draw status to busy
    state.drawStatus = DS_TEXT;

    // setting range
    HAL_TRY(ILI9488_SetRange(spi, x_p, x_p + boundsWidth_p - 1, y_p,
                             y_p + CHARHEIGHT - 1));

    // constants
    state.width_b = boundsWidth_p / 8;
    state.colour =
        (uint32_t)colour | (uint32_t)(colour << 8) | (uint32_t)(colour << 16) |
        (uint32_t)(colour
                   << 24); // setting colour to full map for proper text display
    state.overWrite = overWrite;
    state.text = text;
    state.textSize = textSize;

    // buffer progress cursor data
    state.progress_eb = 0;
    state.target_eb = (boundsWidth_p * CHARHEIGHT) / 2;
    state.activeBuf = 0;

    // background cursor data
    state.bgCol_b = 0;
    state.bgPos_b = (ILI9488_WIDTH_BYTES * y_p) + (x_p / 8);
    state.bgRowSkip_b = ILI9488_WIDTH_BYTES - state.width_b;

    // text cursor data
    state.objPos = 0;
    state.objIdx = 0;   // current character
    state.objCount = 0; // column in bytes

    // write data command
    HAL_TRY(ILI9488_Cmd(spi, 0x2C));

    // setting to data mode
    HAL_GPIO_WritePin(DISPLAY_DC_GPIO_Port, DISPLAY_DC_Pin, GPIO_PIN_SET);

    // selecting spi device
    ILI9488_Select();

    // filling up first chunk
    if (state.target_eb <= CHUNK) {
      if (!overWrite) {
        expandBgToChunk(state.background,
                        (uint32_t *)state.buf[state.activeBuf],
                        state.target_eb / 4, state.bgRowSkip_b, &state.bgPos_b,
                        &state.bgCol_b, state.width_b);
      }
      expandTextToChunk(state.colour, (uint32_t *)state.buf[state.activeBuf],
                        state.target_eb / 4, font, state.text, &state.objIdx,
                        &state.objPos, &state.objCount, CHARWIDTH / 8,
                        state.textSize, overWrite);

      HAL_TRY(HAL_SPI_Transmit_DMA(spi, state.buf[state.activeBuf],
                                   state.target_eb));

      state.progress_eb = state.target_eb;
    } else {
      if (!overWrite) {
        expandBgToChunk(state.background,
                        (uint32_t *)state.buf[state.activeBuf], PX_PER_CHUNK,
                        state.bgRowSkip_b, &state.bgPos_b, &state.bgCol_b,
                        state.width_b);
      }
      expandTextToChunk(state.colour, (uint32_t *)state.buf[state.activeBuf],
                        PX_PER_CHUNK, font, state.text, &state.objIdx,
                        &state.objPos, &state.objCount, CHARWIDTH / 8,
                        state.textSize, overWrite);

      state.progress_eb += CHUNK;
      state.activeBuf = !state.activeBuf;

      // filling up second chunk
      uint32_t remaining_p = (state.target_eb - state.progress_eb) / 4;
      uint32_t clampedChunk_p =
          remaining_p < PX_PER_CHUNK ? remaining_p : PX_PER_CHUNK;
      if (!overWrite) {
        expandBgToChunk(state.background,
                        (uint32_t *)state.buf[state.activeBuf], clampedChunk_p,
                        state.bgRowSkip_b, &state.bgPos_b, &state.bgCol_b,
                        state.width_b);
      }
      expandTextToChunk(state.colour, (uint32_t *)state.buf[state.activeBuf],
                        clampedChunk_p, font, state.text, &state.objIdx,
                        &state.objPos, &state.objCount, CHARWIDTH / 8,
                        state.textSize, overWrite);

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
    if (state.progress_eb >= state.target_eb) {
      ILI9488_Deselect();
      state.drawStatus = DS_NONE;
      return;
    }

    // partial chunk remaining
    else if (state.target_eb - state.progress_eb < CHUNK) {

      HAL_SPI_Transmit_DMA(hspi, state.buf[state.activeBuf],
                           state.target_eb - state.progress_eb);
      // set progress to finished
      state.progress_eb += CHUNK;
    }

    // full chunk remaining
    else {
      // incrementing image progress
      state.progress_eb += CHUNK;
      // transmitting loaded buffer
      HAL_SPI_Transmit_DMA(hspi, state.buf[state.activeBuf], CHUNK);
      // toggling active buffer
      state.activeBuf = !state.activeBuf;

      if (!state.overWrite) {
        expandBgToChunk(state.background,
                        (uint32_t *)state.buf[state.activeBuf], PX_PER_CHUNK,
                        state.bgRowSkip_b, &state.bgPos_b, &state.bgCol_b,
                        state.width_b);
      }

      switch (state.drawStatus) {
      case DS_BG:
        break;
      case DS_IMG:
        expandImgToChunk(state.image, state.colour, state.buf[state.activeBuf],
                         PX_PER_CHUNK, &state.objPos, &state.objCount,
                         &state.objIdx, state.overWrite);
        break;
      case DS_TEXT:
        expandTextToChunk(state.colour, (uint32_t *)state.buf[state.activeBuf],
                          PX_PER_CHUNK, font, state.text, &state.objIdx,
                          &state.objPos, &state.objCount, CHARWIDTH / 8,
                          state.textSize, state.overWrite);
        break;
      }
    }
  }
}
