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

extern const Obj_t objects[149];

// lookup table for possible 4 byte packages
// output of .generate_lookup_table.py
extern const uint32_t pixelTable[256];

// table to map brightness power level values to perceived brightness values
extern const uint8_t brightnessTable[40];

#endif /* INC_TABLES_H_ */
