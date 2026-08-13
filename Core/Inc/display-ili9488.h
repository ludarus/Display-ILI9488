/*
 * display.h
 *
 *  Created on: Jun 23, 2026
 *      Author: Luke Fadel
 *
 *      Type annotations: _p = in pixels, _b = in bytes
 */
#include "character.h"
#include "image.h"
#include "main.h"
#include "stm32f0xx_hal_def.h"
#include "stm32f0xx_hal_tim.h"
#include <stdbool.h>

#ifndef INC_DISPLAY_ILI9488_H_
#define INC_DISPLAY_ILI9488_H_

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

// the size of the buffers in bytes that will store the expanded image data
#define CHUNK 2048

// draw state enums (DS = draw status)
#define DS_NONE 0
#define DS_BG 1
#define DS_IMG 2
#define DS_TEXT 3

// colour enums
// #define COLOR_RED 0b10001000
// #define COLOR_GREEN 0b01000100
// #define COLOR_BLUE 0b00100010
// #define COLOR_PURPLE 0b00100010

// macros to set bits in a bit packed array. Only used for debugging functions
// sets pixel/bit to 1
#define SET_PIXEL(array, bit)                                                  \
  ((array)[(bit) / 8] |= (1u << ((bit) % 8))) // returns void
// sets pixel/bit to 0
#define CLR_PIXEL(arr, bit) ((arr)[(bit) / 8] &= ~(1u << ((bit) % 8)))
// shifting byte to desired bit and masking off the rest of the bit
#define GET_PIXEL(array, bit)                                                  \
  (((array)[(bit) / 8] >> ((bit) % 8)) & 1u) // returns 0u or 1u

// struct to store the current state of image rendering,
// as drawing happens between functions and callbacks so shared state is needed
// Reordered for optimal cache utilization and memory efficiency by claude
typedef struct {
  // state variables
  volatile uint8_t drawStatus;
  // buffer toggle
  volatile uint8_t activeBuf;

  // double buffer. aligned for lookup table casting (uint32_t -> uint8_t)
  uint8_t buf[2][CHUNK] __attribute__((aligned(4)));

  // --- Image transfer geometry, accessed together when setting up a transfer
  uint16_t width; // in bytes

  // --- cursor location when loading bg, image, and text ---
  uint32_t fillBgPos;
  uint16_t fillBgCol;
  uint16_t rowSkipBg;

  uint32_t fillImgPos;
  uint8_t fillImgRem;
  uint32_t fillImgIdx;

  // --- things for live decompiling ---
  uint8_t colour;
  Image_t *image;

  Character_t *font;
  uint8_t *text;
  uint8_t currentChar;
  uint8_t textSize;
  uint16_t textPos_b;
  uint16_t textCol_b;
  uint8_t charWidth_b;

  // --- Progress tracking, accessed together during transfer ---
  uint32_t progress; // in pixels
  uint32_t target;   // in bytes/pixel

  // large bit-packed buffer last: no alignment requirement, so it can
  // safely absorb any odd byte count without forcing padding after it
  uint8_t background[((480 * 320) + 7) / 8]; // in bytes
} ImageTransferState_t;

// public functions
HAL_StatusTypeDef ILI9488_SetBrightness(SPI_HandleTypeDef *spi,
                                        TIM_HandleTypeDef *tim, uint8_t val);
HAL_StatusTypeDef ILI9488_SetBackground_Mono(const Image_t *bg);
HAL_StatusTypeDef ILI9488_SetBackground_Col(SPI_HandleTypeDef *spi,
                                            const Image_t *bg);
HAL_StatusTypeDef ILI9488_Init(SPI_HandleTypeDef *spi,
                               TIM_HandleTypeDef *backlightTimer);
HAL_StatusTypeDef ILI9488_Refresh_Mono(SPI_HandleTypeDef *spi);
HAL_StatusTypeDef ILI9488_LoadImage_Mono(SPI_HandleTypeDef *spi, uint16_t x_p,
                                         uint16_t y_p, const Image_t *image,
                                         bool overWrite, bool bg, bool draw);
HAL_StatusTypeDef
ILI9488_LoadText_Mono(SPI_HandleTypeDef *spi, uint16_t x_p, uint16_t y_p,
                      uint8_t text[], uint8_t textSize, const Character_t *font,
                      const uint8_t fontCount, // number of characters in font
                      const uint8_t charWidth_p, const uint16_t charHeight_p,
                      bool overWrite, bool bg, bool draw);
HAL_StatusTypeDef ILI9488_BlitBackground(SPI_HandleTypeDef *spi);
HAL_StatusTypeDef ILI9488_BlitImage(SPI_HandleTypeDef *spi, uint16_t x_p,
                                    uint16_t y_p, const Image_t *image,
                                    const uint8_t colour);

HAL_StatusTypeDef
ILI9488_BlitText(SPI_HandleTypeDef *spi, uint16_t x_p, uint16_t y_p,
                 uint8_t text[], const uint16_t textSize,
                 const Character_t *font, const uint8_t fontCount,
                 const uint8_t charWidth_p, const uint8_t charHeight_p,
                 const uint8_t colour);

#endif /* INC_DISPLAY_ILI9488_H_ */
