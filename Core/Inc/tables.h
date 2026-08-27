/*
 * tables.h
 *
 *  Created on: 4 Aug 2026
 *      Author: Luke Fadel
 */

#ifndef INC_OBJECTS_H_
#define INC_OBJECTS_H_

// including object types
#include "commands-can.h"

// object table containing location and image information
// output of scripts/generate_object_table.py
extern const Obj_t objects[149];

// lookup table for possible 4 byte packages
// output of scripts/generate_lookup_table.py
extern const uint32_t bgPixelTable[256];

// table to map brightness power level values to perceived brightness values
// output of scripts/generate_brightness_table.py
extern const uint8_t brightnessTable[40];

// table used in masking of coloured font characters
// should remain constant, no need for altering
extern const uint16_t nibbleTable[16];

#endif /* INC_TABLES_H_ */
