/*
 * display-ili9488.h
 *
 *  Created on: 24 Aug 2026
 *      Author: Luke Fadel
 */

#include "image.h"
#include "main.h"
#include <stdbool.h>

#ifndef INC_DISPLAY_ILI9488_H_
#define INC_DISPLAY_ILI9488_H_

// flag to enable or disable colour
#define COLOUR_ENABLED 1

// width and height of display in pixels
#define ILI9488_WIDTH_PX 480
#define ILI9488_HEIGHT_PX 320

#define ILI9488_WIDTH_BYTES (ILI9488_WIDTH_PX / 8)

// size of page in flash
// previously defined in stm32f0xx_hal_flash_ex.h:150:9
// 2048 bytes
#define FLASH_PAGE_SIZE 0x800U

// the address that contains the first value in the last page of flash
#define BRIGHTNESS_PAGE_ADDR 0x0803F800

// the size in bytes of the SPI transmission buffers
#define CHUNK 2048
#define PX_PER_CHUNK (CHUNK / 4)

// draw state enums (DS = draw status)
#define DS_NONE 0
#define DS_BG 1
#define DS_IMG 2
#define DS_TEXT 3

// command enums

// colour enums
#define COLOR_BLACK (uint8_t)0b00000000
#define COLOR_WHITE (uint8_t)0b00111111
#define COLOR_RED (uint8_t)0b00100100
#define COLOR_GREEN (uint8_t)0b00010010
#define COLOR_BLUE (uint8_t)0b00001001
#define COLOR_PURPLE (uint8_t)0b00101101
#define COLOR_CYAN (uint8_t)0b00011011
#define COLOR_YELLOW (uint8_t)0b00110110

// public driver functions

HAL_StatusTypeDef ILI9488_SetBrightness(SPI_HandleTypeDef *spi,
                                        TIM_HandleTypeDef *tim, uint8_t idx);
HAL_StatusTypeDef ILI9488_SetBackground(SPI_HandleTypeDef *spi,
                                        const Image_t *bg);
HAL_StatusTypeDef ILI9488_Init(SPI_HandleTypeDef *spi,
                               TIM_HandleTypeDef *backlightTimer);

#if COLOUR_ENABLED

// chunk is in expanded bytes, 2px = 1eb
// struct to store the current state of object rendering,
typedef struct {
  // drawing state
  volatile uint8_t drawStatus;
  // buffer toggle
  volatile uint8_t activeBuf;

  // double buffer. aligned for lookup table casting (uint32_t -> uint8_t)
  uint8_t buf[2][CHUNK] __attribute__((aligned(4)));

  // should be equal to the width of the draw region of the object in bytes
  uint16_t width_b;

  // cursor variables
  // the column of the cursor for background in bytes
  uint16_t bgCol_b;
  // equal to the screen width - draw width in bytes
  uint16_t bgRowSkip_b;
  // background position in bytes
  uint32_t bgPos_b;

  // progress and target cursors measured in expanded bytes
  // 1 expanded byte holds 2 pixels
  uint32_t progress_eb;
  uint32_t target_eb;

  // for img idx, current char. Should be initialized to 0 on draw
  uint32_t objIdx;
  // for text position, img position. Should be initialized to 0 on draw
  uint32_t objPos;
  // for img remaining, text col. Should be initialized to imgData[0], or 0 on
  // draw depending on if it's text or an image
  uint16_t objCount;

  // boolean to overwrite the background or not
  bool overWrite;

  // use 32 bit mask for text, otherwise cast to 8 bit version for images
  uint32_t colour;

  Image_t *image;

  uint8_t *text;
  // the number of characters in the text array
  uint8_t textSize;

  // large bit-packed buffer last: no alignment requirement, so it can
  // safely absorb any odd byte count without forcing padding after it
  uint8_t background[((480 * 320) + 7) / 8]; // in bytes
} ImageTransferState_t;

HAL_StatusTypeDef ILI9488_BlitImage(SPI_HandleTypeDef *spi, uint16_t x_p,
                                    uint16_t y_p, const Image_t *image,
                                    const bool overWrite, const uint8_t colour);
HAL_StatusTypeDef ILI9488_BlitText(SPI_HandleTypeDef *spi, uint16_t x_p,
                                   uint16_t y_p, uint8_t text[],
                                   uint16_t textSize, const bool overWrite,
                                   const uint8_t colour);

#else

// struct to store the current state of image rendering,
// as drawing happens between functions and callbacks so shared state is needed
// Reordered for optimal cache utilization and memory efficiency by claude
typedef struct {
  // drawing state
  volatile uint8_t drawStatus;

  // buffer toggle
  volatile uint8_t activeBuf;

  // double buffer. aligned for lookup table casting (uint32_t -> uint8_t)
  uint8_t buf[2][CHUNK] __attribute__((aligned(4)));

  // --- Image transfer geometry, accessed together when setting up a transfer
  uint16_t x;      // in bytes
  uint16_t y;      // in pixels
  uint16_t width;  // in bytes
  uint16_t height; // in pixels

  // --- cursor location when loading image ---
  uint32_t fillPos;
  uint16_t fillCol;
  uint16_t rowSkip;

  // --- background image ---
  Image_t *backgroundImage;

  // --- Progress tracking, accessed together during transfer ---
  // progress and target cursors measured in expanded bytes
  // 1 expanded byte holds 2 pixels
  volatile uint32_t progress_eb; // in pixels
  uint32_t target_eb;            // in bytes/pixel
  uint32_t objSize_p;            // in pixels

  // large bit-packed buffer last: no alignment requirement, so it can
  // safely absorb any odd byte count without forcing padding after it
  uint8_t screenCopy[((480 * 320) + 7) / 8]; // in bytes
} ImageTransferState_t;

HAL_StatusTypeDef ILI9488_BlitImage(SPI_HandleTypeDef *spi, uint16_t x_p,
                                    uint16_t y_p, const Image_t *image,
                                    const bool overWrite);
HAL_StatusTypeDef ILI9488_BlitText(SPI_HandleTypeDef *spi, uint16_t x_p,
                                   uint16_t y_p, uint8_t text[],
                                   uint16_t textSize, const bool overWrite);
#endif /* COLOUR_ENABLED */

#endif /* INC_DISPLAY_ILI9488_H_ */
