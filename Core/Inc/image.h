/*
 * image.h
 *
 *  Created on: Jun 18, 2026
 *      Author: Luke Fadel
 */

#ifndef INC_IMAGE_H_
#define INC_IMAGE_H_

#include <stdint.h>

// struct to store image data
typedef struct {
  // width of image in pixels
  const uint16_t width;
  // height of image in pixels
  const uint16_t height;
  // rle encoded string of RLE compressed data
  const uint8_t *data;
  // size of RLE compressed image in bytes
  const uint32_t size;
} Image_t;

#endif /* INC_IMAGE_H_ */
