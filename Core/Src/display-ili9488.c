/*
 * display-ili9488.c
 */

#include "display-ili9488.h"
#include "font.h"
#include "tables.h"

//--------------------------------------------------------------------------------
// driver explanation

/* --- GENERAL (Both Monochrome and Coloured) --- */

/* Main Functions:
 *
 *	Hardware Abstraction
 * - ILI9488_Reset()
 *	  - hardware resets the display using the reset pin
 *	- ILI9488_Select()
 *	  - Selects display spi device using the CS pin, active low
 *	- ILI9488_Deselect()
 *	  - Deselects display spi device using the CS pin, active high
 *	- ILI9488_Cmd()
 *	  - Sends command via spi to display
 *	- ILI9488_Data()
 *	  - Sends data via spi to display
 *	- ILI9488_SetRange()
 *	  - Sets writing region of the display
 *
 *	Initialization
 * - ILI9488_BrightnessInit()
 *   - Parses last page of flash for saved brightness index/setting
 *   - Returns settings to main Init function
 * - ILI9488_Init()
 *   - Initializes display and display settings
 *   - Sets background to empty image
 *
 * Setters
 * - ILI9488_SetBackground()
 *   - Sets the background of the canvas to a background object and draws it to
 * the display in overwrite mode
 * - ILI9488_SetBrightness()
 *   - Sets brightness of display to a specified brightness index

 * General Info / Problem:
 * - SPI Baud = 12Mbit/sec
 *   - To get fastest transfer use DMA and lowest colour resolution
 * - Colour resolution = 3bit per pixel (effectively 4 bit per pixel due to
 * padding)
 * - Due to SRAM limitations (32KiB), a full resolution colour screen buffer
 * cannot be stored in RAM (or even flash)
 * - Store images with RLE commpression in flash (256KiB) and decompress when
 * needed
 * - Expand compressed display info into chunks and send them chunk by chunk

 * Approach:
 * For fast transfer, use a chunking double buffer approach:
 * 1. Fill the first buffer with full resolution colour info
 * 	- make it a small enough chunk so two arrays can fit in RAM
 * 2. Start transferring the first buffer
 * 3. While the first buffer is transferring, start filling the second buffer
 * with pixel data
 * 4. Start transferring the second buffer
 * 5. Repeat until data is fully transferred
 *
 * - The exact methods used to fill the buffer differ between the coloured and
 * monochrome driver variants, detailed below:
 */

/**/

/* --- MONOCHROME --- */

/* Approach:
* - A bitpacked screen copy buffer is stored in global state
* - Type ImageTransferState_t
* - This buffer is synced with the display
* - Any updates to be made to the display are first made to this array
*  	 - via BlitImage() and BlitText() functions
* - Then the ILI9488_Draw() function will read from this array and
expand the bitpacked monochrome data into the double buffer as explained above
using the bgPixelTable lookup table
*  - this lookup table takes a bitpacked byte (uint8_t) and outputs
4 bytes (uint32_t) in full colour resolution, as each full resolution byte
stores 2 pixels
* - note that the state struct must correctly be updated between these function
calls, as this is how data is transferred between the load function, draw
function, and interrupt

* Main Functions:
* - ILI9488_BlitImage()
*   - decomresses an image from flash and loads it onto the screencopy buffer
* - ILI9488_BlitText()
*   - loads a series of characters from the fontmap onto the screencopy buffer
* - ILI9488_Draw()
*   - draws the last updated region
*   - note: a refresh() function that updates the entire screen can be made if
needed by setting state.x and state.y to 0, state.width and state.height to the
dimensions of the screen, and state.objSize to height * width (and calling the
draw function)
* - HAL_SPI_TxCpltCallback()
*   - triggers when a dma transfer over spi has been completed
*   - fills the next buffer with new expanded colour info and transfers the
completed buffer

*/

/* --- COLOURED --- */

/* Approach:
* - A bitpacked background buffer is stored in the global state
* 		- Type ImageTransferState_t
* - This buffer holds whatever the current background is, and is updated when
the background is set (by decompressing the RLE flash bg and bitpacking it into
the buffer)
*   	- SetBackground() function
* - Because there isn't enough RAM to store full colour data, the coloured
objects are live decompressed while the double buffer is transmitting
*   - this results in slower display times on full screen images
* - On a blitImage or blitText call, the correct region of the bitpacked
background buffer is expanded to the double buffer
*	- then, the image/text is decompresed from flash and written on top of
the expanded background (in the double buffer) in full colour resolution
* - because of this, image/text data is actively decompressed in the interrupt
instead of just expanded with the lookup table like in the monochrome variant

* Main Functions:
* - ILI9488_BlitImage()
* 	-	expands the first chunk of background from the bitpacked
background buffer to the first buffer and decompresses the first chunk of image
data on top
* - ILI9488_BlitText()
*   - loads a series of characters from the fontmap onto the screencopy buffer
* 	-	expands the first chunk of background from the bitpacked
background buffer to the first buffer and decompresses the first chunk of text
data on top
* - HAL_SPI_TxCpltCallback()
*   - triggers when a dma transfer over spi has been completed
*   - fills the next double buffer with expanded background info
*   - Decompresses the next image/text data and puts it over the background (if
OR mode is enabled)
         - Transmits the filled buffer
*/

//--------------------------------------------------------------------------------
// global variables

// state so main functions and callbacks can all access render state
static ImageTransferState_t state;

//--------------------------------------------------------------------------------
// private inline utility functions:

// walks through RLE encoded image
// pass a RLE encoded array to walk through,
// the number of remaining pixels (reference so it gets incremented),
// the current index/position of the array (reference),
// and the number of pixels to walk through
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

  // updating references
  (*index) = idx;
  (*remaining_p) = rem;
}

// bitpacks a contiguous section of a buffer while advancing position pointer
// pass a buffer to bitpack,
// the current position within the buffer (reference),
// the number of pixels to bitpack,
// an on or off value of the pixels,
// and an overwrite flag to overwrite the existing ON pixels or not
static inline void fillBitpacked(uint8_t *buf_p, uint32_t *pos_p,
                                 uint16_t count_p, const bool isOn,
                                 const bool overWrite) {
  // pointer aliasing
  uint32_t pos = (*pos_p);

  // filling leading bits
  // if the current position isn't byte aligned
  if (pos % 8 != 0) {
    // compute offsets and a mask to fill the correct part of the byte
    uint8_t offset = pos % 8;
    uint8_t leading = 8 - offset;
    leading = leading > count_p ? count_p : leading;
    uint8_t mask = (uint8_t)(((1u << leading) - 1u) << offset);

    // if the pixels to write are on, OR with existing pixels
    if (isOn) {
      buf_p[pos >> 3] |= mask;
    }
    // if pixels to write are off but overwrite mode is enabled, overwrite
    else if (overWrite) {
      buf_p[pos >> 3] &= (uint8_t)~mask;
    }

    // decrementing count and incrementing position
    // (decrement count for bytes calculation)
    count_p -= leading;
    pos += leading;
  }

  // filling middle bytes
  for (uint8_t byte = 0; byte < count_p >> 3; byte++) {

    // if the pixels to write are on, OR with existing pixels
    if (isOn) {
      // write byte
      buf_p[pos >> 3] = 0xFF;
    }
    // if pixels to write are off but overwrite mode is enabled, overwrite
    else if (overWrite) {
      // clear byte
      buf_p[pos >> 3] = 0;
    }

    // incrementing global position by a byte
    pos += 8;
  }

  // filling trailing bits
  uint8_t trailing = count_p % 8;
  // check for unaligned bits
  if (trailing != 0) {
    // if the pixels to write are on, OR with existing pixels
    if (isOn) {
      buf_p[pos >> 3] |= 0xFF >> (8 - trailing);
    }
    // if pixels to write are off but overwrite mode is enabled, overwrite
    else if (overWrite) {
      buf_p[pos >> 3] &= 0xFF << trailing;
    }

    // increment position
    pos += trailing;
  }

  // setting position reference to updated position
  (*pos_p) = pos;
}

#if COLOUR_ENABLED
// replaces sections of an expanded background with image coloured pixels
// used on the double buffer
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
// used to expand the background buffer to the double buffer
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

// expands compressed rle image data to the double buffer
// ORing with background in or mode, and overwriting previous
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
    // expanding the data to the double buffer
    fillExpanded(buf_p, &pos, chunk, colour, idx % 2, overWrite);
    // advancing the rle pointers
    rleAdvance(imgData, &rem, &idx, chunk);
  }

  // updating references
  (*pos_p) = pos;
  (*rem_p) = rem;
  (*index) = idx;
}

// helper function to get the correct mask from the nibble table
static inline uint32_t fontMask(uint8_t glyphByte) {
  return (uint32_t)nibbleTable[glyphByte & 0x0F] |
         ((uint32_t)nibbleTable[glyphByte >> 4] << 16);
}

// expands compressed bitpacked text data to the double buffer
// ORing with background in or mode, and overwriting previous
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
    // getting mask for bitpacked text
    uint32_t mask = fontMask(font[text[cChar] - 32].data[pos + col]);

    if (overWrite) {
      // replacing bytes of buffer with correct colour data (4 bytes at a time)
      buf_b[i] = mask & expandedColour;
    } else {
      // ORing bytes of buffer with correct colour data (4 bytes at a time)
      buf_b[i] = (buf_b[i] & ~mask) | (expandedColour & mask);
    }

    // incrementing counters
    if (++col >= charWidth_b) {
      col = 0;
      cChar++;
      if (cChar >= textSize) {
        // switching to next character if this one is finished
        pos += charWidth_b;
        cChar = 0;
      }
    }
  }

  // updating references
  *currentChar = cChar;
  *pos_b = pos;
  *col_b = col;
}

#else
// expands a bitpacked byte to the proper data format & colour resolution to
// transmit to the display.
// Takes 1 byte which contains 8 pixels worth of information and expands it into
// 4 bytes of colour coded data
// used to expand bytes in the screencopy buffer to the chunked double buffer
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

// preloads a rle encoded background image to the screencopy buffer on an
// inputted region
// used for OR mode drawing to make sure data from the previous
// write isn't kept
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

// hardware resets the display using the reset pin
void ILI9488_Reset(void) {
  // setting the reset pin to low to signal a reset
  HAL_GPIO_WritePin(DISPLAY_RESET_GPIO_Port, DISPLAY_RESET_Pin, GPIO_PIN_RESET);

  // small delay
  HAL_Delay(10);

  // setting the pin to high (default state)
  HAL_GPIO_WritePin(DISPLAY_RESET_GPIO_Port, DISPLAY_RESET_Pin, GPIO_PIN_SET);

  HAL_Delay(100);
}

// Selects display spi device using the CS pin, active low
void ILI9488_Select(void) {
  // setting the select pin to low
  HAL_GPIO_WritePin(DISPLAY_CS_GPIO_Port, DISPLAY_CS_Pin, GPIO_PIN_RESET);
}

// Deselects display spi device using the CS pin, active high
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
  HAL_TRY(ILI9488_Cmd(spi, DCMD_CASET));

  // parameters: starting col MSB, starting col LSB, ending col MSB, ending
  // col LSB
  uint8_t caset[] = {

      (uint8_t)(colStart >> 8),

      (uint8_t)(colStart & 0xFF),

      (uint8_t)(colEnd >> 8),

      (uint8_t)(colEnd & 0xFF)

  };

  HAL_TRY(ILI9488_Data(spi, &caset[0], 4));

  // set row address command
  HAL_TRY(ILI9488_Cmd(spi, DCMD_PASET));
  // parameters: starting row MSB, starting row LSB, ending row MSB, ending
  // row LSB
  uint8_t paset[] = {

      (uint8_t)(rowStart >> 8),

      (uint8_t)(rowStart & 0xFF),

      (uint8_t)(rowEnd >> 8),

      (uint8_t)(rowEnd & 0xFF)};

  HAL_TRY(ILI9488_Data(spi, &paset[0], 4));

  return HAL_OK;
}

// Parses last page of flash for saved brightness index/setting
// Returns settings to main Init function
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

  ILI9488_SetBrightness(spi, backlightTimer, DEFAULT_BRIGHTNESS_INDEX);

  return output;
}

#if COLOUR_ENABLED
// replaces background buffer in state with new one
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
    HAL_TRY(ILI9488_Cmd(spi, DCMD_RAMWR));

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

  // checking if the inputted write region exceeds the display's bounds
  // conditions should never trigger, but still here just in case
  if (state.x + state.width > ILI9488_WIDTH_PX) {
    state.width = ILI9488_WIDTH_PX - state.x;
  }

  if (state.y + state.height > ILI9488_HEIGHT_PX) {
    state.height = ILI9488_HEIGHT_PX - state.y;
  }

  // converting pixels to bytes because the rest of the function uses bytes
  state.x /= 8;
  state.width /= 8;

  // write data command
  HAL_TRY(ILI9488_Cmd(spi, DCMD_RAMWR));

  // setting to data mode
  HAL_GPIO_WritePin(DISPLAY_DC_GPIO_Port, DISPLAY_DC_Pin, GPIO_PIN_SET);

  // selecting spi device
  ILI9488_Select();

  // sending image data. chunking data for DMA and memory saving purposes

  // setting progress counter to 0
  state.progress_eb = 0;

  // setting image target (in expanded bytes)
  state.target_eb = state.objSize_p / 2;

  // setting the active buffer in the double buffer
  state.activeBuf = 0;

  // setting the starting position of the fill cursor in bytes
  state.fillPos_b = (uint32_t)ILI9488_WIDTH_BYTES * state.y + state.x;
  // setting the starting column relative to the write region of the fill cursor
  // in bytes
  state.fillCol_b = 0;
  // setting the number of bytes to add when the cursor has reached the last
  // column of the write region
  state.rowSkip_b = ILI9488_WIDTH_BYTES - state.width;

  // expanding and transmitting the first chunk of the write region

  // if the target is smaller than the maximum chunk value, transmit only the
  // size of the target
  if (state.target_eb <= CHUNK) {
    // expanding the target amount of screencopy to the first double buffer
    expandToChunk(state.screenCopy, (uint32_t *)state.buf[state.activeBuf],
                  state.target_eb >> 2, state.rowSkip_b, &state.fillPos_b,
                  &state.fillCol_b, state.width);
    // transmission
    HAL_TRY(
        HAL_SPI_Transmit_DMA(spi, state.buf[state.activeBuf], state.target_eb));

    // condition to finish transmitting in the interrupt
    state.progress_eb = state.target_eb;
  } else {
    // expanding a full CHUNK of the screencopy buffer to the first double
    // buffer
    expandToChunk(state.screenCopy, (uint32_t *)state.buf[state.activeBuf],
                  CHUNK >> 2, state.rowSkip_b, &state.fillPos_b,
                  &state.fillCol_b, state.width);

    // incrementing progress counter
    state.progress_eb += CHUNK;

    // toggling the active buffer
    state.activeBuf = !state.activeBuf;

    uint32_t remaining = state.target_eb - state.progress_eb;

    // expanding the next chunk of data from screencopy to the second double
    // buffer
    expandToChunk(state.screenCopy, (uint32_t *)state.buf[state.activeBuf],
                  // checking if the remaining data is smaller than a chunk
                  (remaining < CHUNK ? remaining : CHUNK) >> 2,

                  state.rowSkip_b, &state.fillPos_b, &state.fillCol_b,
                  state.width);
    // transmitting the first double buffer that has already been filled
    HAL_TRY(HAL_SPI_Transmit_DMA(spi, state.buf[!state.activeBuf], CHUNK));
  }
  return HAL_OK;
}
#endif

//--------------------------------------------------------------------------------
// public display driver functions

#if COLOUR_ENABLED

// starts drawing image to screen (processing continues in interrupt)
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
    HAL_TRY(ILI9488_Cmd(spi, DCMD_RAMWR));

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

// starts drawing text to screen (processing continues in interrupt)
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
    HAL_TRY(ILI9488_Cmd(spi, DCMD_RAMWR));

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
  // checking if another function is already drawing
  if (state.drawStatus == DS_NONE) {
    // checking to make sure the image is in bounds:
    if (x_p + image->width > ILI9488_WIDTH_PX ||
        y_p + image->height > ILI9488_HEIGHT_PX) {
      return HAL_ERROR;
    }

    // setting draw status in case other functions are called while this one is
    // operating on the screen buffer
    state.drawStatus = DS_IMG;

    // copying state variables for memory optimization (pointer aliasing)

    // column of image relative to the starting column of the image
    uint16_t col_p = 0; // in pixels
                        // global position of the loading cursor  (index)
    uint32_t pos_p = (ILI9488_WIDTH_PX * y_p) + x_p;          // in pixels
    const uint16_t imgWidth_p = image->width;                 // in pixels
    const uint16_t imgHeight_p = image->height;               // in pixels
    const uint16_t rowSkip_p = ILI9488_WIDTH_PX - imgWidth_p; // in pixels
    const uint8_t *imgData = image->data;
    uint8_t *screenData = state.screenCopy;

    // pre loading background if OR mode is enabled
    if (!overWrite) {
      // replacing previous display info for this region with background
      bgPreload(state.backgroundImage->data, screenData, pos_p, imgWidth_p,
                imgHeight_p);
    }

    // iterating through every RLE value
    for (uint32_t i = 0; i < image->size; i++) {

      // RLE on or off value
      bool isOn = i % 2;

      // remaining number of pixels to load in this RLE value
      uint8_t remaining_p = imgData[i]; // in pixels

      while (remaining_p > 0) {
        // Sets the current chunk to the largest possible contiguous segment of
        // pixels in the current row
        uint16_t chunk_p = imgWidth_p - col_p;

        // clamping chunk to the number of remaining pixels
        chunk_p = remaining_p > chunk_p ? chunk_p : remaining_p;

        // decrementing the number of remaining pixels
        remaining_p -= chunk_p;

        // filling the screenbuffer with this segment of pixels
        // note: increments pos_p within the function
        fillBitpacked(screenData, &pos_p, chunk_p, isOn, overWrite);

        // incrementing column counter
        // should only ever == imgWidth as per the logic above
        if ((col_p += chunk_p) >= imgWidth_p) {
          // if the edge of the image has been reached, reset column and
          // increment position by one row
          col_p = 0;
          pos_p += rowSkip_p;
        }
      }
    }

    // setting state variables for DRAW function write region
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
  // checking if another function is already drawing
  if (state.drawStatus == DS_NONE) {

    // full width of text
    uint16_t boundsWidth_p = CHARWIDTH * textSize; // in pixels

    // checking to make sure the minimum text is in bounds of the display
    if (y_p + CHARHEIGHT > ILI9488_HEIGHT_PX || x_p > ILI9488_WIDTH_PX) {
      return HAL_ERROR;
    }

    // setting draw status
    state.drawStatus = DS_TEXT;

    // clamping text size to only what can fit on screen
    if (boundsWidth_p + x_p > ILI9488_WIDTH_PX) {
      textSize = (ILI9488_WIDTH_PX - x_p) / CHARWIDTH;
      boundsWidth_p = CHARWIDTH * textSize;
    }

    // text processing to make sure all characters are displayable
    // (eg. if they aren't represented in the fontmap, replace with blank
    // character)
    for (uint8_t i = 0; i < textSize; i++) {
      // if not displayable set to blank character
      if (text[i] < 32 || text[i] >= FONTSIZE + 32) {
        text[i] = 32;
      }
    }

    // defining constants for text loading
    const uint32_t bytesPerChar_b = (CHARWIDTH * CHARHEIGHT) / 8; // in bytes
    const uint8_t charWidth_b = CHARWIDTH / 8;                    // in bytes
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

    // setting state variables for draw function
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

// sets brightness of display to a specified brightness index
HAL_StatusTypeDef ILI9488_SetBrightness(SPI_HandleTypeDef *spi,
                                        TIM_HandleTypeDef *tim, uint8_t idx) {
  uint8_t val = brightnessTable[idx];
  // for pwm driven brightness
  __HAL_TIM_SET_COMPARE(tim, TIM_CHANNEL_1, val);

  // for displays without a physical brightness pin
  HAL_TRY(ILI9488_Cmd(spi, DCMD_WRDISBV));
  HAL_TRY(ILI9488_Data(spi, &val, 1));

  return HAL_OK;
}

// Sets the background of the canvas to a background object and draws it to the
// display in overwrite mode
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

// Initializes display and display settings
// Sets background to empty image
BrightnessInfo_t ILI9488_Init(SPI_HandleTypeDef *spi,
                              TIM_HandleTypeDef *backlightTimer) {

  // hardware reset
  ILI9488_Reset();
  HAL_Delay(100);

  // software reset
  ILI9488_Cmd(spi, DCMD_SWRESET);
  HAL_Delay(100);

  // exit sleep mode
  ILI9488_Cmd(spi, DCMD_SLPOUT);
  HAL_Delay(10);

  // TODO remove this in production
  HAL_GPIO_WritePin(SWITCH_POWER_GPIO_Port, SWITCH_POWER_Pin, GPIO_PIN_SET);

  // backlight on
  // starting display backlight pwm timer
  HAL_TIM_PWM_Start(backlightTimer, TIM_CHANNEL_1);

  // configuring extended command set for spi write
  ILI9488_Cmd(spi, DCMD_ADJCTRL3);

  uint8_t unlockData[] = {0xA9, 0x51, 0x2C, 0x82};
  ILI9488_Data(spi, &unlockData[0], 4);

  // memory data access control - instruction 36h MADCTL
  ILI9488_Cmd(spi, DCMD_MADCTL);
  uint8_t madctl = 0b00101000;
  ILI9488_Data(spi, &madctl, 1);

  // configuring brightness control settings - instruction 53h WRCTRLD
  ILI9488_Cmd(spi, DCMD_WRCTRLD);
  uint8_t brightnessCtl = 0b00101100;
  ILI9488_Data(spi, &brightnessCtl, 1);

  // Interface Pixel Format - instruction 3Ah COLMOD
  ILI9488_Cmd(spi, DCMD_COLMOD);
  // Lowest available is 3bit/pixel
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

  // enable gamma altering
  ILI9488_Cmd(spi, DCMD_ADJCTRL4);
  uint8_t ctrl4[2] = {0x21, 0b00000110};
  ILI9488_Data(spi, &ctrl4[0], 2);

  // enable colour profiles
  ILI9488_Cmd(spi, DCMD_ADJCTRL7);
  uint8_t ctrl7 = 0b11000010;
  ILI9488_Data(spi, &ctrl7, 1);

  // gamma profile
  ILI9488_Cmd(spi, DCMD_PGAMCTRL);
  // uint8_t pgamma[15] = {0x00, 0x03, 0x09, 0x08, 0x16, 0x0A, 0x3F, 0x78,
  //                       0x4C, 0x09, 0x0A, 0x08, 0x16, 0x1A, 0x0F};
  uint8_t pgamma[15] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  ILI9488_Data(spi, &pgamma[0], 15);

  ILI9488_Cmd(spi, DCMD_NGAMCTRL);
  // uint8_t ngamma[15] = {0x00, 0x16, 0x19, 0x03, 0x0F, 0x05, 0x32, 0x45,
  //                       0x46, 0x04, 0x0E, 0x0D, 0x35, 0x37, 0x0F};
  uint8_t ngamma[15] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  ILI9488_Data(spi, &ngamma[0], 15);

  // VCOM value for more display tweaks
  ILI9488_Cmd(spi, DCMD_VMCTRL);
  uint8_t vcom[3] = {0x00, 0x12, 0x80};
  ILI9488_Data(spi, &vcom[0], 3);

  // colour enhancement
  ILI9488_Cmd(spi, DCMD_CECTRL1);
  uint8_t cectrl1[12] = {
      0x04, 0x04, 0x04, 0x04, // First_Axis  (Red). Has seemingly no effect
      0x00, 0x00, 0x00, 0x00, // Second_Axis (Yellow). Has seemingly no effect
      0x1F, 0x1F, 0x1F, 0x1F, // Third_Axis  (Green). Confirmed green
  };
  ILI9488_Data(spi, &cectrl1[0], 12);

  ILI9488_Cmd(spi, DCMD_CECTRL2);
  uint8_t cectrl2[12] = {
      0x04, 0x04, 0x04, 0x04, // Fourth_Axis (Cyan) Seemingly only effects blue
      0x04, 0x04, 0x04, 0x04, // Fifth_Axis (Blue) Has seemingly no effect
      0x1F, 0x1F, 0x1F, 0x1F, // Sixth_Axis (Magenta) Seemingly only effects Red
  };
  ILI9488_Data(spi, &cectrl2[0], 12);

  // coarse gamma adjustment
  ILI9488_Cmd(spi, DCMD_DGAMCTRL1);
  uint8_t gamctrl1[16] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  ILI9488_Data(spi, &gamctrl1[0], 16);

  // micro gamma adjustment
  ILI9488_Cmd(spi, DCMD_DGAMCTRL2);
  uint8_t gamctrl2[64] = {
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  };
  ILI9488_Data(spi, &gamctrl2[0], 64);
  //
  // enabling display inversion for IPS display
  ILI9488_Cmd(spi, DCMD_INVON);
  // no data

  // display on
  ILI9488_Cmd(spi, DCMD_DISON);

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
      // transmitting the rest of the info
      HAL_SPI_Transmit_DMA(hspi, state.buf[state.activeBuf],
                           state.target_eb - state.progress_eb);
      // set progress to finished
      state.progress_eb += CHUNK;
    }

    // full chunk remaining
    else {
      // incrementing image progress
      state.progress_eb += CHUNK;
      // transmitting the loaded buffer from last cycle
      HAL_SPI_Transmit_DMA(hspi, state.buf[state.activeBuf], CHUNK);
      // toggling active buffer
      state.activeBuf = !state.activeBuf;

      // expanding the next CHUNK of screencopy
      expandToChunk(state.screenCopy, (uint32_t *)state.buf[state.activeBuf],
                    CHUNK >> 2, state.rowSkip_b, &state.fillPos_b,
                    &state.fillCol_b, state.width);
    }
  }
}
#endif
