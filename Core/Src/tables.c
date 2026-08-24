/*
 * tables.c
 *
 *  Created on: 4 Aug 2026
 *      Author: Luke Fadel
 */

// includes for every image
#include "tables.h"
#include "File_002_ObjNum_001_NEW_6_17_26.h"
#include "File_005_ObjNum_004_480x320_6_18_26.h"
#include "File_006_ObjNum_005_480x320_6_18_26_C.h"
#include "File_007_ObjNum_006_480x320_6_18_26.h"
#include "File_008_ObjNum_007_96x190_6_18_26.h"
#include "File_009_ObjNum_008_96x190_6_18_26.h"
#include "File_010_ObjNum_009_96x190_6_18_26.h"
#include "File_011_ObjNum_010_96x190_6_18_26.h"
#include "File_012_ObjNum_011_96x190_6_18_26.h"
#include "File_013_ObjNum_012_96x190_6_18_26.h"
#include "File_014_ObjNum_013_96x190_6_18_26.h"
#include "File_015_ObjNum_014_96x190_6_18_26.h"
#include "File_016_ObjNum_015_96x190_6_18_26.h"
#include "File_017_ObjNum_016_96x190_6_18_26.h"
#include "File_018_ObjNum_017_96x190_6_16_26.h"
#include "File_019_ObjNum_018_48x208_6_19_26.h"
#include "File_020_ObjNum_019_48x208_6_19_26.h"
#include "File_021_ObjNum_020_48x208_6_19_26.h"
#include "File_022_ObjNum_021_48x208_6_19_26.h"
#include "File_023_ObjNum_022_48x208_6_19_26.h"
#include "File_024_ObjNum_023_48x208_6_19_26.h"
#include "File_025_ObjNum_024_48x208_6_19_26.h"
#include "File_026_ObjNum_025_48x208_6_19_26.h"
#include "File_027_ObjNum_026_48x208_6_19_26.h"
#include "File_028_ObjNum_027_48x208_6_19_26.h"
#include "File_029_ObjNum_028_48x208_6_19_26.h"
#include "File_030_ObjNum_029_64x25_6_19_26.h"
#include "File_031_ObjNum_030_64x25_6_19_26.h"
#include "File_032_ObjNum_031_96x190_6_19_26.h"
#include "File_033_ObjNum_032_96x190_6_19_26.h"
#include "File_034_ObjNum_033_96x190_6_19_26.h"
#include "File_035_ObjNum_034_96x190_6_19_26.h"
#include "File_036_ObjNum_035_96x190_6_19_26.h"
#include "File_037_ObjNum_036_96x190_6_19_26.h"
#include "File_038_ObjNum_037_96x190_6_19_26.h"
#include "File_039_ObjNum_038_96x190_6_19_26.h"
#include "File_040_ObjNum_039_96x190_6_19_26.h"
#include "File_041_ObjNum_040_96x190_6_19_26.h"
#include "File_042_ObjNum_041_96x190_6_19_26.h"
#include "File_043_ObjNum_042_144x208_6_19_26.h"
#include "File_044_ObjNum_043_144x208_6_19_26.h"
#include "File_045_ObjNum_044_144x208_6_19_26.h"
#include "File_046_ObjNum_045_144x208_6_19_26.h"
#include "File_047_ObjNum_046_144x208_6_19_26.h"
#include "File_048_ObjNum_052_144x183_6_19_26.h"
#include "File_049_ObjNum_053_144x183_6_19_26.h"
#include "File_050_ObjNum_054_144x183_6_19_26.h"
#include "File_051_ObjNum_055_144x183_6_19_26.h"
#include "File_052_ObjNum_057_144x30_6_19_26.h"
#include "File_053_ObjNum_077_48x30_6_19_26.h"
#include "File_054_ObjNum_087_48x255_6_19_26.h"
#include "File_055_ObjNum_090_480x320_6_18_26.h"
#include "File_056_ObjNum_091_480x320_6_18_26.h"
#include "File_057_ObjNum_092_480x320_6_18_26.h"
#include "File_058_ObjNum_093_480x320_6_18_26.h"
#include "File_059_ObjNum_094_480x320_6_18_26.h"
#include "File_062_ObjNum_097_480x40_6_19_26.h"
#include "File_063_ObjNum_098_176x33_6_19_26.h"
#include "File_064_ObjNum_099_48x63_6_19_26.h"
#include "File_065_ObjNum_104_16x30_6_19_26.h"
#include "File_066_ObjNum_109_96x80_6_19_26.h"
#include "File_067_ObjNum_114_64x103_6_19_26.h"
#include "File_068_ObjNum_119_64x103_6_19_26.h"
#include "File_069_ObjNum_124_64x103_6_19_26.h"
#include "File_070_ObjNum_129_96x80_6_19_26.h"
#include "File_071_ObjNum_134_176x188_6_19_26.h"
#include "File_072_ObjNum_135_480x320_6_18_26.h"
#include "File_073_ObjNum_136_480x320_6_18_26.h"
#include "File_074_ObjNum_138_48x143_6_19_26.h"
#include "File_075_ObjNum_140_368x40_6_19_26.h"
#include "File_076_ObjNum_143_368x40_6_19_26.h"
#include "File_077_ObjNum_147_480x320_6_18_26.h"
#include "File_078_ObjNum_148_480x320_6_18_26.h"
#include "File_079_ObjNum_149_480x320_6_17_26.h"
#include "commands-can.h"
#include "display-ili9488-colour.h"

const Obj_t objects[] = {
    {1, BACKGROUND_OBJ_TYPE, 0, 0, COLOR_YELLOW,
     &File_002_ObjNum_001_NEW_6_17_26},
    {
        2,
        IMAGE_OBJ_TYPE,
        0,
        0,
        COLOR_GREEN,
    },
    {
        3,
        UNKNOWN_OBJ_TYPE,
        0,
        0,
        COLOR_YELLOW,
    },
    {4, BACKGROUND_OBJ_TYPE, 0, 0, COLOR_YELLOW,
     &File_005_ObjNum_004_480x320_6_18_26},
    {5, BACKGROUND_OBJ_TYPE, 0, 0, COLOR_YELLOW,
     &File_006_ObjNum_005_480x320_6_18_26_C},
    {6, BACKGROUND_OBJ_TYPE, 0, 0, COLOR_YELLOW,
     &File_007_ObjNum_006_480x320_6_18_26},
    {7, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_008_ObjNum_007_96x190_6_18_26},
    {8, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_009_ObjNum_008_96x190_6_18_26},
    {9, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_010_ObjNum_009_96x190_6_18_26},
    {10, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_011_ObjNum_010_96x190_6_18_26},
    {11, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_012_ObjNum_011_96x190_6_18_26},
    {12, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_013_ObjNum_012_96x190_6_18_26},
    {13, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_014_ObjNum_013_96x190_6_18_26},
    {14, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_015_ObjNum_014_96x190_6_18_26},
    {15, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_016_ObjNum_015_96x190_6_18_26},
    {16, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_017_ObjNum_016_96x190_6_18_26},
    {17, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_018_ObjNum_017_96x190_6_16_26},
    {18, IMAGE_OBJ_TYPE, 432, 48, COLOR_GREEN,
     &File_019_ObjNum_018_48x208_6_19_26},
    {19, IMAGE_OBJ_TYPE, 432, 48, COLOR_GREEN,
     &File_020_ObjNum_019_48x208_6_19_26},
    {20, IMAGE_OBJ_TYPE, 432, 48, COLOR_GREEN,
     &File_021_ObjNum_020_48x208_6_19_26},
    {21, IMAGE_OBJ_TYPE, 432, 48, COLOR_GREEN,
     &File_022_ObjNum_021_48x208_6_19_26},
    {22, IMAGE_OBJ_TYPE, 432, 48, COLOR_GREEN,
     &File_023_ObjNum_022_48x208_6_19_26},
    {23, IMAGE_OBJ_TYPE, 432, 48, COLOR_GREEN,
     &File_024_ObjNum_023_48x208_6_19_26},
    {24, IMAGE_OBJ_TYPE, 432, 48, COLOR_GREEN,
     &File_025_ObjNum_024_48x208_6_19_26},
    {25, IMAGE_OBJ_TYPE, 432, 48, COLOR_GREEN,
     &File_026_ObjNum_025_48x208_6_19_26},
    {26, IMAGE_OBJ_TYPE, 432, 48, COLOR_GREEN,
     &File_027_ObjNum_026_48x208_6_19_26},
    {27, IMAGE_OBJ_TYPE, 432, 48, COLOR_GREEN,
     &File_028_ObjNum_027_48x208_6_19_26},
    {28, IMAGE_OBJ_TYPE, 432, 48, COLOR_GREEN,
     &File_029_ObjNum_028_48x208_6_19_26},
    {29, IMAGE_OBJ_TYPE, 408, 48, COLOR_GREEN,
     &File_030_ObjNum_029_64x25_6_19_26},
    {30, IMAGE_OBJ_TYPE, 408, 48, COLOR_GREEN,
     &File_031_ObjNum_030_64x25_6_19_26},
    {31, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_032_ObjNum_031_96x190_6_19_26},
    {32, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_033_ObjNum_032_96x190_6_19_26},
    {33, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_034_ObjNum_033_96x190_6_19_26},
    {34, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_035_ObjNum_034_96x190_6_19_26},
    {35, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_036_ObjNum_035_96x190_6_19_26},
    {36, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_037_ObjNum_036_96x190_6_19_26},
    {37, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_038_ObjNum_037_96x190_6_19_26},
    {38, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_039_ObjNum_038_96x190_6_19_26},
    {39, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_040_ObjNum_039_96x190_6_19_26},
    {40, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_041_ObjNum_040_96x190_6_19_26},
    {41, IMAGE_OBJ_TYPE, 0, 56, COLOR_GREEN,
     &File_042_ObjNum_041_96x190_6_19_26},
    {42, IMAGE_OBJ_TYPE, 96, 48, COLOR_GREEN,
     &File_043_ObjNum_042_144x208_6_19_26},
    {43, IMAGE_OBJ_TYPE, 96, 48, COLOR_GREEN,
     &File_044_ObjNum_043_144x208_6_19_26},
    {44, IMAGE_OBJ_TYPE, 96, 48, COLOR_GREEN,
     &File_045_ObjNum_044_144x208_6_19_26},
    {45, IMAGE_OBJ_TYPE, 96, 48, COLOR_GREEN,
     &File_046_ObjNum_045_144x208_6_19_26},
    {46, IMAGE_OBJ_TYPE, 96, 48, COLOR_GREEN,
     &File_047_ObjNum_046_144x208_6_19_26},
    {47, IMAGE_OBJ_TYPE, 96, 48, COLOR_GREEN,
     &File_043_ObjNum_042_144x208_6_19_26},
    {48, IMAGE_OBJ_TYPE, 96, 48, COLOR_GREEN,
     &File_044_ObjNum_043_144x208_6_19_26},
    {49, IMAGE_OBJ_TYPE, 96, 48, COLOR_GREEN,
     &File_045_ObjNum_044_144x208_6_19_26},
    {50, IMAGE_OBJ_TYPE, 96, 48, COLOR_GREEN,
     &File_046_ObjNum_045_144x208_6_19_26},
    {51, IMAGE_OBJ_TYPE, 96, 48, COLOR_GREEN,
     &File_047_ObjNum_046_144x208_6_19_26},
    {52, IMAGE_OBJ_TYPE, 96, 48, COLOR_GREEN,
     &File_048_ObjNum_052_144x183_6_19_26},
    {53, IMAGE_OBJ_TYPE, 96, 48, COLOR_GREEN,
     &File_049_ObjNum_053_144x183_6_19_26},
    {54, IMAGE_OBJ_TYPE, 96, 48, COLOR_GREEN,
     &File_050_ObjNum_054_144x183_6_19_26},
    {55, IMAGE_OBJ_TYPE, 96, 48, COLOR_GREEN,
     &File_051_ObjNum_055_144x183_6_19_26},
    {56, IMAGE_OBJ_TYPE, 96, 48, COLOR_GREEN,
     &File_049_ObjNum_053_144x183_6_19_26},
    {57, IMAGE_OBJ_TYPE, 240, 48, COLOR_GREEN,
     &File_052_ObjNum_057_144x30_6_19_26},
    {
        58,
        TEXT_OBJ_TYPE,
        0,
        4,
        COLOR_CYAN,
    },
    {
        59,
        TEXT_OBJ_TYPE,
        240,
        4,
        COLOR_CYAN,
    },
    {
        60,
        TEXT_OBJ_TYPE,
        408,
        4,
        COLOR_CYAN,
    },
    {
        61,
        TEXT_OBJ_TYPE,
        0,
        284,
        COLOR_CYAN,
    },
    {
        62,
        TEXT_OBJ_TYPE,
        264,
        284,
        COLOR_CYAN,
    },
    {
        63,
        TEXT_OBJ_TYPE,
        0,
        4,
        COLOR_CYAN,
    },
    {
        64,
        TEXT_OBJ_TYPE,
        0,
        84,
        COLOR_CYAN,
    },
    {
        65,
        TEXT_OBJ_TYPE,
        0,
        164,
        COLOR_CYAN,
    },
    {
        66,
        TEXT_OBJ_TYPE,
        0,
        244,
        COLOR_CYAN,
    },
    {
        67,
        TEXT_OBJ_TYPE,
        48,
        64,
        COLOR_CYAN,
    },
    {
        68,
        TEXT_OBJ_TYPE,
        48,
        116,
        COLOR_CYAN,
    },
    {
        69,
        TEXT_OBJ_TYPE,
        48,
        168,
        COLOR_CYAN,
    },
    {
        70,
        TEXT_OBJ_TYPE,
        48,
        220,
        COLOR_CYAN,
    },
    {
        71,
        TEXT_OBJ_TYPE,
        48,
        272,
        COLOR_CYAN,
    },
    {
        72,
        TEXT_OBJ_TYPE,
        288,
        64,
        COLOR_CYAN,
    },
    {
        73,
        TEXT_OBJ_TYPE,
        288,
        116,
        COLOR_CYAN,
    },
    {
        74,
        TEXT_OBJ_TYPE,
        288,
        168,
        COLOR_CYAN,
    },
    {
        75,
        TEXT_OBJ_TYPE,
        288,
        220,
        COLOR_CYAN,
    },
    {
        76,
        TEXT_OBJ_TYPE,
        288,
        272,
        COLOR_CYAN,
    },
    {77, IMAGE_OBJ_TYPE, 0, 64, COLOR_GREEN,
     &File_053_ObjNum_077_48x30_6_19_26},
    {78, IMAGE_OBJ_TYPE, 0, 116, COLOR_GREEN,
     &File_053_ObjNum_077_48x30_6_19_26},
    {79, IMAGE_OBJ_TYPE, 0, 168, COLOR_GREEN,
     &File_053_ObjNum_077_48x30_6_19_26},
    {80, IMAGE_OBJ_TYPE, 0, 220, COLOR_GREEN,
     &File_053_ObjNum_077_48x30_6_19_26},
    {81, IMAGE_OBJ_TYPE, 0, 272, COLOR_GREEN,
     &File_053_ObjNum_077_48x30_6_19_26},
    {82, IMAGE_OBJ_TYPE, 240, 64, COLOR_GREEN,
     &File_053_ObjNum_077_48x30_6_19_26},
    {83, IMAGE_OBJ_TYPE, 240, 116, COLOR_GREEN,
     &File_053_ObjNum_077_48x30_6_19_26},
    {84, IMAGE_OBJ_TYPE, 240, 168, COLOR_GREEN,
     &File_053_ObjNum_077_48x30_6_19_26},
    {85, IMAGE_OBJ_TYPE, 240, 220, COLOR_GREEN,
     &File_053_ObjNum_077_48x30_6_19_26},
    {86, IMAGE_OBJ_TYPE, 240, 272, COLOR_GREEN,
     &File_053_ObjNum_077_48x30_6_19_26},
    {87, IMAGE_OBJ_TYPE, 0, 48, COLOR_GREEN,
     &File_054_ObjNum_087_48x255_6_19_26},
    {88, IMAGE_OBJ_TYPE, 240, 48, COLOR_GREEN,
     &File_054_ObjNum_087_48x255_6_19_26},
    {
        89,
        TEXT_OBJ_TYPE,
        0,
        244,
        COLOR_CYAN,
    },
    {90, IMAGE_OBJ_TYPE, 0, 0, COLOR_GREEN,
     &File_055_ObjNum_090_480x320_6_18_26},
    {91, IMAGE_OBJ_TYPE, 0, 0, COLOR_GREEN,
     &File_056_ObjNum_091_480x320_6_18_26},
    {92, IMAGE_OBJ_TYPE, 0, 0, COLOR_GREEN,
     &File_057_ObjNum_092_480x320_6_18_26},
    {93, IMAGE_OBJ_TYPE, 0, 0, COLOR_GREEN,
     &File_058_ObjNum_093_480x320_6_18_26},
    {94, IMAGE_OBJ_TYPE, 0, 0, COLOR_GREEN,
     &File_059_ObjNum_094_480x320_6_18_26},
    {
        95,
        GROUPTABLE_OBJ_TYPE,
        0,
        0,
        COLOR_YELLOW,
    },
    {
        96,
        GROUPTABLE_OBJ_TYPE,
        0,
        0,
        COLOR_YELLOW,
    },
    {97, IMAGE_OBJ_TYPE, 0, 244, COLOR_GREEN,
     &File_062_ObjNum_097_480x40_6_19_26},
    {98, IMAGE_OBJ_TYPE, 240, 200, COLOR_GREEN,
     &File_063_ObjNum_098_176x33_6_19_26},
    {99, IMAGE_OBJ_TYPE, 264, 232, COLOR_GREEN,
     &File_064_ObjNum_099_48x63_6_19_26},
    {100, IMAGE_OBJ_TYPE, 288, 232, COLOR_GREEN,
     &File_064_ObjNum_099_48x63_6_19_26},
    {101, IMAGE_OBJ_TYPE, 312, 232, COLOR_GREEN,
     &File_064_ObjNum_099_48x63_6_19_26},
    {102, IMAGE_OBJ_TYPE, 336, 232, COLOR_GREEN,
     &File_064_ObjNum_099_48x63_6_19_26},
    {103, IMAGE_OBJ_TYPE, 360, 232, COLOR_GREEN,
     &File_064_ObjNum_099_48x63_6_19_26},
    {104, IMAGE_OBJ_TYPE, 264, 232, COLOR_GREEN,
     &File_065_ObjNum_104_16x30_6_19_26},
    {105, IMAGE_OBJ_TYPE, 288, 232, COLOR_GREEN,
     &File_065_ObjNum_104_16x30_6_19_26},
    {106, IMAGE_OBJ_TYPE, 312, 232, COLOR_GREEN,
     &File_065_ObjNum_104_16x30_6_19_26},
    {107, IMAGE_OBJ_TYPE, 336, 232, COLOR_GREEN,
     &File_065_ObjNum_104_16x30_6_19_26},
    {108, IMAGE_OBJ_TYPE, 360, 232, COLOR_GREEN,
     &File_065_ObjNum_104_16x30_6_19_26},
    {109, IMAGE_OBJ_TYPE, 240, 116, COLOR_GREEN,
     &File_066_ObjNum_109_96x80_6_19_26},
    {110, IMAGE_OBJ_TYPE, 264, 116, COLOR_GREEN,
     &File_066_ObjNum_109_96x80_6_19_26},
    {111, IMAGE_OBJ_TYPE, 288, 116, COLOR_GREEN,
     &File_066_ObjNum_109_96x80_6_19_26},
    {112, IMAGE_OBJ_TYPE, 312, 116, COLOR_GREEN,
     &File_066_ObjNum_109_96x80_6_19_26},
    {113, IMAGE_OBJ_TYPE, 336, 116, COLOR_GREEN,
     &File_066_ObjNum_109_96x80_6_19_26},
    {114, IMAGE_OBJ_TYPE, 240, 92, COLOR_GREEN,
     &File_067_ObjNum_114_64x103_6_19_26},
    {115, IMAGE_OBJ_TYPE, 264, 92, COLOR_GREEN,
     &File_067_ObjNum_114_64x103_6_19_26},
    {116, IMAGE_OBJ_TYPE, 288, 92, COLOR_GREEN,
     &File_067_ObjNum_114_64x103_6_19_26},
    {117, IMAGE_OBJ_TYPE, 312, 92, COLOR_GREEN,
     &File_067_ObjNum_114_64x103_6_19_26},
    {118, IMAGE_OBJ_TYPE, 336, 92, COLOR_GREEN,
     &File_067_ObjNum_114_64x103_6_19_26},
    {119, IMAGE_OBJ_TYPE, 240, 92, COLOR_GREEN,
     &File_068_ObjNum_119_64x103_6_19_26},
    {120, IMAGE_OBJ_TYPE, 264, 92, COLOR_GREEN,
     &File_068_ObjNum_119_64x103_6_19_26},
    {121, IMAGE_OBJ_TYPE, 288, 92, COLOR_GREEN,
     &File_068_ObjNum_119_64x103_6_19_26},
    {122, IMAGE_OBJ_TYPE, 312, 92, COLOR_GREEN,
     &File_068_ObjNum_119_64x103_6_19_26},
    {123, IMAGE_OBJ_TYPE, 336, 92, COLOR_GREEN,
     &File_068_ObjNum_119_64x103_6_19_26},
    {124, IMAGE_OBJ_TYPE, 240, 92, COLOR_GREEN,
     &File_069_ObjNum_124_64x103_6_19_26},
    {125, IMAGE_OBJ_TYPE, 264, 92, COLOR_GREEN,
     &File_069_ObjNum_124_64x103_6_19_26},
    {126, IMAGE_OBJ_TYPE, 288, 92, COLOR_GREEN,
     &File_069_ObjNum_124_64x103_6_19_26},
    {127, IMAGE_OBJ_TYPE, 312, 92, COLOR_GREEN,
     &File_069_ObjNum_124_64x103_6_19_26},
    {128, IMAGE_OBJ_TYPE, 336, 92, COLOR_GREEN,
     &File_069_ObjNum_124_64x103_6_19_26},
    {129, IMAGE_OBJ_TYPE, 240, 116, COLOR_GREEN,
     &File_070_ObjNum_129_96x80_6_19_26},
    {130, IMAGE_OBJ_TYPE, 264, 116, COLOR_GREEN,
     &File_070_ObjNum_129_96x80_6_19_26},
    {131, IMAGE_OBJ_TYPE, 288, 116, COLOR_GREEN,
     &File_070_ObjNum_129_96x80_6_19_26},
    {132, IMAGE_OBJ_TYPE, 312, 116, COLOR_GREEN,
     &File_070_ObjNum_129_96x80_6_19_26},
    {133, IMAGE_OBJ_TYPE, 336, 116, COLOR_GREEN,
     &File_070_ObjNum_129_96x80_6_19_26},
    {134, IMAGE_OBJ_TYPE, 240, 88, COLOR_GREEN,
     &File_071_ObjNum_134_176x188_6_19_26},
    {135, BACKGROUND_OBJ_TYPE, 0, 0, COLOR_YELLOW,
     &File_072_ObjNum_135_480x320_6_18_26},
    {136, BACKGROUND_OBJ_TYPE, 0, 0, COLOR_YELLOW,
     &File_073_ObjNum_136_480x320_6_18_26},
    {
        137,
        TEXT_OBJ_TYPE,
        144,
        236,
        COLOR_CYAN,
    },
    {138, IMAGE_OBJ_TYPE, 0, 168, COLOR_GREEN,
     &File_074_ObjNum_138_48x143_6_19_26},
    {139, IMAGE_OBJ_TYPE, 240, 168, COLOR_GREEN,
     &File_074_ObjNum_138_48x143_6_19_26},
    {140, IMAGE_OBJ_TYPE, 0, 176, COLOR_GREEN,
     &File_075_ObjNum_140_368x40_6_19_26},
    {141, IMAGE_OBJ_TYPE, 0, 224, COLOR_GREEN,
     &File_075_ObjNum_140_368x40_6_19_26},
    {142, IMAGE_OBJ_TYPE, 0, 272, COLOR_GREEN,
     &File_075_ObjNum_140_368x40_6_19_26},
    {143, IMAGE_OBJ_TYPE, 0, 176, COLOR_GREEN,
     &File_076_ObjNum_143_368x40_6_19_26},
    {144, IMAGE_OBJ_TYPE, 0, 224, COLOR_GREEN,
     &File_076_ObjNum_143_368x40_6_19_26},
    {145, IMAGE_OBJ_TYPE, 0, 272, COLOR_GREEN,
     &File_076_ObjNum_143_368x40_6_19_26},
    {
        146,
        TEXT_OBJ_TYPE,
        96,
        244,
        COLOR_CYAN,
    },
    {147, BACKGROUND_OBJ_TYPE, 0, 0, COLOR_YELLOW,
     &File_077_ObjNum_147_480x320_6_18_26},
    {148, BACKGROUND_OBJ_TYPE, 0, 0, COLOR_YELLOW,
     &File_078_ObjNum_148_480x320_6_18_26},
    {149, BACKGROUND_OBJ_TYPE, 0, 0, COLOR_YELLOW,
     &File_079_ObjNum_149_480x320_6_17_26},
};
// lookup table for possible 4 byte packages
// output of .generate_lookup_table.py

const uint32_t bgPixelTable[256] = {
    0x00000000, 0x00000030, 0x00000006, 0x00000036, 0x00003000, 0x00003030,
    0x00003006, 0x00003036, 0x00000600, 0x00000630, 0x00000606, 0x00000636,
    0x00003600, 0x00003630, 0x00003606, 0x00003636, 0x00300000, 0x00300030,
    0x00300006, 0x00300036, 0x00303000, 0x00303030, 0x00303006, 0x00303036,
    0x00300600, 0x00300630, 0x00300606, 0x00300636, 0x00303600, 0x00303630,
    0x00303606, 0x00303636, 0x00060000, 0x00060030, 0x00060006, 0x00060036,
    0x00063000, 0x00063030, 0x00063006, 0x00063036, 0x00060600, 0x00060630,
    0x00060606, 0x00060636, 0x00063600, 0x00063630, 0x00063606, 0x00063636,
    0x00360000, 0x00360030, 0x00360006, 0x00360036, 0x00363000, 0x00363030,
    0x00363006, 0x00363036, 0x00360600, 0x00360630, 0x00360606, 0x00360636,
    0x00363600, 0x00363630, 0x00363606, 0x00363636, 0x30000000, 0x30000030,
    0x30000006, 0x30000036, 0x30003000, 0x30003030, 0x30003006, 0x30003036,
    0x30000600, 0x30000630, 0x30000606, 0x30000636, 0x30003600, 0x30003630,
    0x30003606, 0x30003636, 0x30300000, 0x30300030, 0x30300006, 0x30300036,
    0x30303000, 0x30303030, 0x30303006, 0x30303036, 0x30300600, 0x30300630,
    0x30300606, 0x30300636, 0x30303600, 0x30303630, 0x30303606, 0x30303636,
    0x30060000, 0x30060030, 0x30060006, 0x30060036, 0x30063000, 0x30063030,
    0x30063006, 0x30063036, 0x30060600, 0x30060630, 0x30060606, 0x30060636,
    0x30063600, 0x30063630, 0x30063606, 0x30063636, 0x30360000, 0x30360030,
    0x30360006, 0x30360036, 0x30363000, 0x30363030, 0x30363006, 0x30363036,
    0x30360600, 0x30360630, 0x30360606, 0x30360636, 0x30363600, 0x30363630,
    0x30363606, 0x30363636, 0x06000000, 0x06000030, 0x06000006, 0x06000036,
    0x06003000, 0x06003030, 0x06003006, 0x06003036, 0x06000600, 0x06000630,
    0x06000606, 0x06000636, 0x06003600, 0x06003630, 0x06003606, 0x06003636,
    0x06300000, 0x06300030, 0x06300006, 0x06300036, 0x06303000, 0x06303030,
    0x06303006, 0x06303036, 0x06300600, 0x06300630, 0x06300606, 0x06300636,
    0x06303600, 0x06303630, 0x06303606, 0x06303636, 0x06060000, 0x06060030,
    0x06060006, 0x06060036, 0x06063000, 0x06063030, 0x06063006, 0x06063036,
    0x06060600, 0x06060630, 0x06060606, 0x06060636, 0x06063600, 0x06063630,
    0x06063606, 0x06063636, 0x06360000, 0x06360030, 0x06360006, 0x06360036,
    0x06363000, 0x06363030, 0x06363006, 0x06363036, 0x06360600, 0x06360630,
    0x06360606, 0x06360636, 0x06363600, 0x06363630, 0x06363606, 0x06363636,
    0x36000000, 0x36000030, 0x36000006, 0x36000036, 0x36003000, 0x36003030,
    0x36003006, 0x36003036, 0x36000600, 0x36000630, 0x36000606, 0x36000636,
    0x36003600, 0x36003630, 0x36003606, 0x36003636, 0x36300000, 0x36300030,
    0x36300006, 0x36300036, 0x36303000, 0x36303030, 0x36303006, 0x36303036,
    0x36300600, 0x36300630, 0x36300606, 0x36300636, 0x36303600, 0x36303630,
    0x36303606, 0x36303636, 0x36060000, 0x36060030, 0x36060006, 0x36060036,
    0x36063000, 0x36063030, 0x36063006, 0x36063036, 0x36060600, 0x36060630,
    0x36060606, 0x36060636, 0x36063600, 0x36063630, 0x36063606, 0x36063636,
    0x36360000, 0x36360030, 0x36360006, 0x36360036, 0x36363000, 0x36363030,
    0x36363006, 0x36363036, 0x36360600, 0x36360630, 0x36360606, 0x36360636,
    0x36363600, 0x36363630, 0x36363606, 0x36363636};

// table to map brightness power level values to perceived brightness values
const uint8_t brightnessTable[] = {
    1,   2,   3,   4,   6,   9,   12,  15,  18,  22,  26,  31,  35,  40,
    46,  51,  57,  63,  69,  76,  83,  90,  97,  105, 112, 120, 128, 137,
    146, 154, 164, 173, 182, 192, 202, 212, 223, 233, 244, 255,
};

// spreads a 4-bit nibble into 4 output nibbles (0xF where set, 0x0 where clear)
const uint16_t nibbleTable[16] = {
    0x0000, 0x000F, 0x00F0, 0x00FF, 0x0F00, 0x0F0F, 0x0FF0, 0x0FFF,
    0xF000, 0xF00F, 0xF0F0, 0xF0FF, 0xFF00, 0xFF0F, 0xFFF0, 0xFFFF,
};
