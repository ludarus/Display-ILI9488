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
#include "display-ili9488.h"

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
        5,
        COLOR_CYAN,
    },
    {
        59,
        TEXT_OBJ_TYPE,
        240,
        5,
        COLOR_CYAN,
    },
    {
        60,
        TEXT_OBJ_TYPE,
        408,
        5,
        COLOR_CYAN,
    },
    {
        61,
        TEXT_OBJ_TYPE,
        0,
        285,
        COLOR_CYAN,
    },
    {
        62,
        TEXT_OBJ_TYPE,
        264,
        285,
        COLOR_CYAN,
    },
    {
        63,
        TEXT_OBJ_TYPE,
        0,
        5,
        COLOR_CYAN,
    },
    {
        64,
        TEXT_OBJ_TYPE,
        0,
        85,
        COLOR_CYAN,
    },
    {
        65,
        TEXT_OBJ_TYPE,
        0,
        165,
        COLOR_CYAN,
    },
    {
        66,
        TEXT_OBJ_TYPE,
        0,
        245,
        COLOR_CYAN,
    },
    {
        67,
        TEXT_OBJ_TYPE,
        48,
        65,
        COLOR_CYAN,
    },
    {
        68,
        TEXT_OBJ_TYPE,
        48,
        117,
        COLOR_CYAN,
    },
    {
        69,
        TEXT_OBJ_TYPE,
        48,
        169,
        COLOR_CYAN,
    },
    {
        70,
        TEXT_OBJ_TYPE,
        48,
        221,
        COLOR_CYAN,
    },
    {
        71,
        TEXT_OBJ_TYPE,
        48,
        273,
        COLOR_CYAN,
    },
    {
        72,
        TEXT_OBJ_TYPE,
        288,
        65,
        COLOR_CYAN,
    },
    {
        73,
        TEXT_OBJ_TYPE,
        288,
        117,
        COLOR_CYAN,
    },
    {
        74,
        TEXT_OBJ_TYPE,
        288,
        169,
        COLOR_CYAN,
    },
    {
        75,
        TEXT_OBJ_TYPE,
        288,
        221,
        COLOR_CYAN,
    },
    {
        76,
        TEXT_OBJ_TYPE,
        288,
        273,
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
        245,
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
        237,
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
        245,
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
    0x2D2D2D2D, 0x2D2D2D35, 0x2D2D2D2E, 0x2D2D2D36, 0x2D2D352D, 0x2D2D3535,
    0x2D2D352E, 0x2D2D3536, 0x2D2D2E2D, 0x2D2D2E35, 0x2D2D2E2E, 0x2D2D2E36,
    0x2D2D362D, 0x2D2D3635, 0x2D2D362E, 0x2D2D3636, 0x2D352D2D, 0x2D352D35,
    0x2D352D2E, 0x2D352D36, 0x2D35352D, 0x2D353535, 0x2D35352E, 0x2D353536,
    0x2D352E2D, 0x2D352E35, 0x2D352E2E, 0x2D352E36, 0x2D35362D, 0x2D353635,
    0x2D35362E, 0x2D353636, 0x2D2E2D2D, 0x2D2E2D35, 0x2D2E2D2E, 0x2D2E2D36,
    0x2D2E352D, 0x2D2E3535, 0x2D2E352E, 0x2D2E3536, 0x2D2E2E2D, 0x2D2E2E35,
    0x2D2E2E2E, 0x2D2E2E36, 0x2D2E362D, 0x2D2E3635, 0x2D2E362E, 0x2D2E3636,
    0x2D362D2D, 0x2D362D35, 0x2D362D2E, 0x2D362D36, 0x2D36352D, 0x2D363535,
    0x2D36352E, 0x2D363536, 0x2D362E2D, 0x2D362E35, 0x2D362E2E, 0x2D362E36,
    0x2D36362D, 0x2D363635, 0x2D36362E, 0x2D363636, 0x352D2D2D, 0x352D2D35,
    0x352D2D2E, 0x352D2D36, 0x352D352D, 0x352D3535, 0x352D352E, 0x352D3536,
    0x352D2E2D, 0x352D2E35, 0x352D2E2E, 0x352D2E36, 0x352D362D, 0x352D3635,
    0x352D362E, 0x352D3636, 0x35352D2D, 0x35352D35, 0x35352D2E, 0x35352D36,
    0x3535352D, 0x35353535, 0x3535352E, 0x35353536, 0x35352E2D, 0x35352E35,
    0x35352E2E, 0x35352E36, 0x3535362D, 0x35353635, 0x3535362E, 0x35353636,
    0x352E2D2D, 0x352E2D35, 0x352E2D2E, 0x352E2D36, 0x352E352D, 0x352E3535,
    0x352E352E, 0x352E3536, 0x352E2E2D, 0x352E2E35, 0x352E2E2E, 0x352E2E36,
    0x352E362D, 0x352E3635, 0x352E362E, 0x352E3636, 0x35362D2D, 0x35362D35,
    0x35362D2E, 0x35362D36, 0x3536352D, 0x35363535, 0x3536352E, 0x35363536,
    0x35362E2D, 0x35362E35, 0x35362E2E, 0x35362E36, 0x3536362D, 0x35363635,
    0x3536362E, 0x35363636, 0x2E2D2D2D, 0x2E2D2D35, 0x2E2D2D2E, 0x2E2D2D36,
    0x2E2D352D, 0x2E2D3535, 0x2E2D352E, 0x2E2D3536, 0x2E2D2E2D, 0x2E2D2E35,
    0x2E2D2E2E, 0x2E2D2E36, 0x2E2D362D, 0x2E2D3635, 0x2E2D362E, 0x2E2D3636,
    0x2E352D2D, 0x2E352D35, 0x2E352D2E, 0x2E352D36, 0x2E35352D, 0x2E353535,
    0x2E35352E, 0x2E353536, 0x2E352E2D, 0x2E352E35, 0x2E352E2E, 0x2E352E36,
    0x2E35362D, 0x2E353635, 0x2E35362E, 0x2E353636, 0x2E2E2D2D, 0x2E2E2D35,
    0x2E2E2D2E, 0x2E2E2D36, 0x2E2E352D, 0x2E2E3535, 0x2E2E352E, 0x2E2E3536,
    0x2E2E2E2D, 0x2E2E2E35, 0x2E2E2E2E, 0x2E2E2E36, 0x2E2E362D, 0x2E2E3635,
    0x2E2E362E, 0x2E2E3636, 0x2E362D2D, 0x2E362D35, 0x2E362D2E, 0x2E362D36,
    0x2E36352D, 0x2E363535, 0x2E36352E, 0x2E363536, 0x2E362E2D, 0x2E362E35,
    0x2E362E2E, 0x2E362E36, 0x2E36362D, 0x2E363635, 0x2E36362E, 0x2E363636,
    0x362D2D2D, 0x362D2D35, 0x362D2D2E, 0x362D2D36, 0x362D352D, 0x362D3535,
    0x362D352E, 0x362D3536, 0x362D2E2D, 0x362D2E35, 0x362D2E2E, 0x362D2E36,
    0x362D362D, 0x362D3635, 0x362D362E, 0x362D3636, 0x36352D2D, 0x36352D35,
    0x36352D2E, 0x36352D36, 0x3635352D, 0x36353535, 0x3635352E, 0x36353536,
    0x36352E2D, 0x36352E35, 0x36352E2E, 0x36352E36, 0x3635362D, 0x36353635,
    0x3635362E, 0x36353636, 0x362E2D2D, 0x362E2D35, 0x362E2D2E, 0x362E2D36,
    0x362E352D, 0x362E3535, 0x362E352E, 0x362E3536, 0x362E2E2D, 0x362E2E35,
    0x362E2E2E, 0x362E2E36, 0x362E362D, 0x362E3635, 0x362E362E, 0x362E3636,
    0x36362D2D, 0x36362D35, 0x36362D2E, 0x36362D36, 0x3636352D, 0x36363535,
    0x3636352E, 0x36363536, 0x36362E2D, 0x36362E35, 0x36362E2E, 0x36362E36,
    0x3636362D, 0x36363635, 0x3636362E, 0x36363636};

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
