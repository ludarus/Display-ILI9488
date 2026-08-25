/*
 * display.c
 *
 */

#include "display-ili9488.h"
#include "File_005_ObjNum_004_480x320_6_18_26.h"
#include "character.h"
#include "font.h"
#include "main.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_spi.h"
#include "tables.h"

//--------------------------------------------------------------------------------
// global variables

// state so main functions and callbacks can all access render state
static ImageTransferState_t state;

//--------------------------------------------------------------------------------
// private inline utility functions:

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

#if COLOUR_ENABLED
// replaces sections of an expanded background with image coloured pixels
static inline void fillExpanded(uint8_t *buf, uint32_t *pos_p, uint16_t count_p,
                                const uint8_t onColour, bool isOn,
                                const bool overWrite) {
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

#else
// expands a bitpacked byte to the proper data format & colour resolution to
// transmit to the display.
// Takes 1 byte which contains 8 pixels worth of information and expands it into
// 4 bytes of colour coded data
static inline void expandToChunk(uint8_t *screenData, uint32_t *dst,
                                 const uint32_t count, const uint32_t rowSkip_b,
                                 uint32_t *pos_b, uint16_t *col_b,
                                 const uint16_t width_b) {
  uint32_t pos = *pos_b;
  uint16_t col = *col_b;

  for (uint32_t i = 0; i < count; i++) {
    dst[i] = bgPixelTable[screenData[pos]];
    pos++;
    if (++col == width_b) {
      col = 0;
      pos += rowSkip_b;
    }
  }

  (*pos_b) = pos;
  (*col_b) = col;
}

static inline void bgPreload(const uint8_t *bgData, uint8_t *screenData,
                             uint32_t pos_p, const uint16_t width_p,
                             const uint16_t height_p) {

  const uint16_t rowSkip_p = ILI9488_WIDTH_PX - width_p;

  uint8_t bgRem_p = bgData[0];
  uint32_t bgIdx = 0;

  // getting correct background index for rle value
  rleAdvance(bgData, &bgRem_p, &bgIdx, pos_p);

  for (uint16_t row = 0; row < height_p; row++) {
    uint16_t remainingPx = width_p;

    while (remainingPx > 0) {
      uint16_t chunk = (remainingPx > bgRem_p) ? bgRem_p : remainingPx;
      remainingPx -= chunk;

      fillBitpacked(screenData, &pos_p, chunk, bgIdx % 2, true);

      rleAdvance(bgData, &bgRem_p, &bgIdx, chunk);
    }
    pos_p += rowSkip_p;
    rleAdvance(bgData, &bgRem_p, &bgIdx, rowSkip_p);
  }
}
#endif

//--------------------------------------------------------------------------------
// private display utility functions

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

BrightnessInfo_t ILI9488_BrightnessInit(SPI_HandleTypeDef *spi,
                                        TIM_HandleTypeDef *backlightTimer) {
  BrightnessInfo_t output;
  // reading flash to get last value of pointer
  // two bytes per half word
  for (int32_t offset = FLASH_PAGE_SIZE - 2; offset >= 0; offset -= 2) {
    // checking if the 16 bit half word is smaller than the default value
    // protocol: store the brightness val in the first 8 bits of the halfword,
    // then set the last 8 bits to 0 to indicate that the byte has been written
    if (*(__IO uint16_t *)(offset + BRIGHTNESS_PAGE_ADDR) < 0xFFFF) {
      output.flashOffset = (uint32_t)offset + 2;
      // setting brightness
      output.prevBrightnessIdx =
          *(__IO uint8_t *)(offset + BRIGHTNESS_PAGE_ADDR);

      ILI9488_SetBrightness(spi, backlightTimer, output.prevBrightnessIdx);

      return output;
    }
  }

  // default value if one can't be found in flash
  output.flashOffset = 0;

  ILI9488_SetBrightness(spi, backlightTimer, 29);

  return output;
}

#if COLOUR_ENABLED
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
#else

// x and y should be multiples of 8
// draws last loaded image to screen
HAL_StatusTypeDef ILI9488_Draw(SPI_HandleTypeDef *spi) {
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
  state.progress_eb = 0;

  state.target_eb = state.objSize_p / 2;

  state.activeBuf = 0;

  state.fillPos = (uint32_t)ILI9488_WIDTH_BYTES * state.y + state.x;
  state.fillCol = 0;
  state.rowSkip = ILI9488_WIDTH_BYTES - state.width;

  if (state.target_eb <= CHUNK) {
    expandToChunk(state.screenCopy, (uint32_t *)state.buf[state.activeBuf],
                  state.target_eb >> 2, state.rowSkip, &state.fillPos,
                  &state.fillCol, state.width);
    HAL_TRY(
        HAL_SPI_Transmit_DMA(spi, state.buf[state.activeBuf], state.target_eb));
    state.progress_eb = state.target_eb;
  } else {
    expandToChunk(state.screenCopy, (uint32_t *)state.buf[state.activeBuf],
                  CHUNK >> 2, state.rowSkip, &state.fillPos, &state.fillCol,
                  state.width);
    state.progress_eb += CHUNK;
    state.activeBuf = !state.activeBuf;

    uint32_t remaining = state.target_eb - state.progress_eb;
    expandToChunk(state.screenCopy, (uint32_t *)state.buf[state.activeBuf],
                  (remaining < CHUNK ? remaining : CHUNK) >> 2, state.rowSkip,
                  &state.fillPos, &state.fillCol, state.width);
    HAL_TRY(HAL_SPI_Transmit_DMA(spi, state.buf[!state.activeBuf], CHUNK));
  }
  return HAL_OK;
}
#endif

//--------------------------------------------------------------------------------
// public display driver functions

#if COLOUR_ENABLED

HAL_StatusTypeDef ILI9488_BlitImage(SPI_HandleTypeDef *spi, uint16_t x_p,
                                    uint16_t y_p, const Image_t *image,
                                    const bool overWrite,
                                    const uint8_t colour) {
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
                                   uint16_t textSize, const bool overWrite,
                                   const uint8_t colour) {

  if (state.drawStatus == DS_NONE) {
    // checking to make sure the text is in bounds
    uint16_t boundsWidth_p = CHARWIDTH * textSize;

    if (y_p + CHARHEIGHT > ILI9488_HEIGHT_PX || x_p > ILI9488_WIDTH_PX) {
      return HAL_ERROR;
    }

    // setting draw status to busy
    state.drawStatus = DS_TEXT;

    // clamping text size
    if (boundsWidth_p + x_p > ILI9488_WIDTH_PX) {
      textSize = (ILI9488_WIDTH_PX - x_p) / CHARWIDTH;
      boundsWidth_p = CHARWIDTH * textSize;
    }

    // text processing to make sure all characters are displayable
    for (uint8_t i = 0; i < textSize; i++) {
      // if not displayable set to blank character
      if (text[i] < 32 || text[i] >= FONTSIZE + 32) {
        text[i] = 32;
      }
    }

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
#else
// loads image to screen buffer
HAL_StatusTypeDef ILI9488_BlitImage(SPI_HandleTypeDef *spi, uint16_t x_p,
                                    uint16_t y_p, const Image_t *image,
                                    bool overWrite) {
  if (state.drawStatus == DS_NONE) {
    // checking to make sure the image is in bounds:
    if (x_p + image->width > ILI9488_WIDTH_PX ||
        y_p + image->height > ILI9488_HEIGHT_PX) {
      return HAL_ERROR;
    }

    state.drawStatus = DS_IMG;

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
    if (!overWrite) {
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

        fillBitpacked(screenData, &pos_p, chunk_p, isOn, overWrite);

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
    state.objSize_p = imgWidth_p * imgHeight_p;

    return ILI9488_Draw(spi);
  } else {
    return HAL_BUSY;
  }
}

// loads text with transparent background on OR mode
HAL_StatusTypeDef ILI9488_BlitText(SPI_HandleTypeDef *spi, uint16_t x_p,
                                   uint16_t y_p, uint8_t text[],
                                   uint16_t textSize, const bool overWrite) {

  if (state.drawStatus == DS_NONE) {
    // checking to make sure the text is in bounds
    uint16_t boundsWidth_p = CHARWIDTH * textSize; // in pixels

    if (y_p + CHARHEIGHT > ILI9488_HEIGHT_PX || x_p > ILI9488_WIDTH_PX) {
      return HAL_ERROR;
    }

    state.drawStatus = DS_TEXT;

    // clamping text size
    if (boundsWidth_p + x_p > ILI9488_WIDTH_PX) {
      textSize = (ILI9488_WIDTH_PX - x_p) / CHARWIDTH;
      boundsWidth_p = CHARWIDTH * textSize;
    }

    // text processing to make sure all characters are displayable
    for (uint8_t i = 0; i < textSize; i++) {
      // if not displayable set to blank character
      if (text[i] < 32 || text[i] >= FONTSIZE + 32) {
        text[i] = 32;
      }
    }

    const uint32_t bytesPerChar_b = (CHARWIDTH * CHARHEIGHT) / 8; // in bytes

    const uint8_t charWidth_b = CHARWIDTH / 8; // in bytes
    const uint16_t rowSkip_b = ILI9488_WIDTH_BYTES - charWidth_b;

    uint8_t *screenData = state.screenCopy;

    // pre loading background if OR mode is enabled
    if (!overWrite) {
      bgPreload(state.backgroundImage->data, screenData,
                (y_p * ILI9488_WIDTH_PX) + x_p, boundsWidth_p, CHARHEIGHT);
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
          (text[charIdx] < 32 || (size_t)(text[charIdx] - 32) >= FONTSIZE)
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
    state.height = CHARHEIGHT;
    state.objSize_p = boundsWidth_p * CHARHEIGHT;

    return ILI9488_Draw(spi);
  } else {
    return HAL_BUSY;
  }
}
#endif

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

#if COLOUR_ENABLED
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
  return ILI9488_BlitBackground(spi);

#else
  // drawing the background
  state.backgroundImage = (Image_t *)bg;
  return ILI9488_BlitImage(spi, 0, 0, bg, true);

#endif
}

BrightnessInfo_t ILI9488_Init(SPI_HandleTypeDef *spi,
                              TIM_HandleTypeDef *backlightTimer) {

  // hardware reset
  ILI9488_Reset();
  HAL_Delay(100);

  // software reset
  ILI9488_Cmd(spi, 0x01);
  HAL_Delay(100);

  // exit sleep mode
  ILI9488_Cmd(spi, 0x11);
  HAL_Delay(10);

  // TODO remove this in production
  HAL_GPIO_WritePin(SWITCH_POWER_GPIO_Port, SWITCH_POWER_Pin, GPIO_PIN_SET);

  // backlight on
  // starting display backlight pwm timer
  HAL_TIM_PWM_Start(backlightTimer, TIM_CHANNEL_1);

  // configuring extended command set for spi write
  ILI9488_Cmd(spi, 0xF7);

  // uint8_t unlockData[] = {0xA9, 0x51, 0x2C, 0x82};
  // HAL_TRY(ILI9488_Data(spi, &unlockData[0], 4));

  // memory data access control - instruction 36h MADCTL
  ILI9488_Cmd(spi, 0x36);
  // same as st7796s
  // 0b00101001
  uint8_t madctl = 0x28;
  ILI9488_Data(spi, &madctl, 1);

  // configuring brightness control settings - instruction 53h WRCTRLD
  ILI9488_Cmd(spi, 0x53);
  // 0 0 1 0 1 1 0 0
  uint8_t brightnessCtl = 0x2C;
  ILI9488_Data(spi, &brightnessCtl, 1);

  // Interface Pixel Format - instruction 3Ah COLMOD
  ILI9488_Cmd(spi, 0x3A);
  // Lowest available is 3bit/pixel
  // 00000001
  // format: 0 0 R G B R G B
  // each byte = two pixels due to padding
  uint8_t colmod = 0b01000001;
  ILI9488_Data(spi, &colmod, 1);

  // Column inversion for display longevity
  // update: seems to cause flicker on this display?
  // 1 dot mode
  //	ILI9488_CMD(spi, 0xB4);
  //	uint8_t inversion = 0x01;
  //	ILI9488_DATA(spi, &inversion, 1);

  // enabling display inversion for IPS display
  ILI9488_Cmd(spi, 0x21);
  // no data

  // enabling partial mode
  ILI9488_Cmd(spi, 0x12);

  // display on
  ILI9488_Cmd(spi, 0x29);

  // initializing background to empty image
  ILI9488_SetBackground(spi, (Image_t *)&File_005_ObjNum_004_480x320_6_18_26);

  return ILI9488_BrightnessInit(spi, backlightTimer);
}

//--------------------------------------------------------------------------------
// DMA callback override

#if COLOUR_ENABLED

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
#else
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
      // can't do anything about error handling here?
      // maybe TODO find a way to re run the draw function if error occurs here
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

      expandToChunk(state.screenCopy, (uint32_t *)state.buf[state.activeBuf],
                    CHUNK >> 2, state.rowSkip, &state.fillPos, &state.fillCol,
                    state.width);
    }
  }
}
#endif
