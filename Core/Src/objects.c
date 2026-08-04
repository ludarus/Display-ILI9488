/*
 * objects.c
 *
 *  Created on: 4 Aug 2026
 *      Author: Luke Fadel
 */

// includes for every image
#include "objects.h"
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

const Obj_t objects[] = {
    {1, BACKGROUND_OBJ_TYPE, 0, 0, &File_002_ObjNum_001_NEW_6_17_26},
    {
        2,
        IMAGE_OBJ_TYPE,
        0,
        0,
    }, // font image
    {
        3,
        UNKNOWN_OBJ_TYPE,
        0,
        0,
    },
    {4, BACKGROUND_OBJ_TYPE, 0, 0, &File_005_ObjNum_004_480x320_6_18_26},
    {5, BACKGROUND_OBJ_TYPE, 0, 0, &File_006_ObjNum_005_480x320_6_18_26_C},
    {6, BACKGROUND_OBJ_TYPE, 0, 0, &File_007_ObjNum_006_480x320_6_18_26},
    {7, IMAGE_OBJ_TYPE, 0, 56, &File_008_ObjNum_007_96x190_6_18_26},
    {8, IMAGE_OBJ_TYPE, 0, 56, &File_009_ObjNum_008_96x190_6_18_26},
    {9, IMAGE_OBJ_TYPE, 0, 56, &File_010_ObjNum_009_96x190_6_18_26},
    {10, IMAGE_OBJ_TYPE, 0, 56, &File_011_ObjNum_010_96x190_6_18_26},
    {11, IMAGE_OBJ_TYPE, 0, 56, &File_012_ObjNum_011_96x190_6_18_26},
    {12, IMAGE_OBJ_TYPE, 0, 56, &File_013_ObjNum_012_96x190_6_18_26},
    {13, IMAGE_OBJ_TYPE, 0, 56, &File_014_ObjNum_013_96x190_6_18_26},
    {14, IMAGE_OBJ_TYPE, 0, 56, &File_015_ObjNum_014_96x190_6_18_26},
    {15, IMAGE_OBJ_TYPE, 0, 56, &File_016_ObjNum_015_96x190_6_18_26},
    {16, IMAGE_OBJ_TYPE, 0, 56, &File_017_ObjNum_016_96x190_6_18_26},
    {17, IMAGE_OBJ_TYPE, 0, 56, &File_018_ObjNum_017_96x190_6_16_26},
    {18, IMAGE_OBJ_TYPE, 432, 48, &File_019_ObjNum_018_48x208_6_19_26},
    {19, IMAGE_OBJ_TYPE, 432, 48, &File_020_ObjNum_019_48x208_6_19_26},
    {20, IMAGE_OBJ_TYPE, 432, 48, &File_021_ObjNum_020_48x208_6_19_26},
    {21, IMAGE_OBJ_TYPE, 432, 48, &File_022_ObjNum_021_48x208_6_19_26},
    {22, IMAGE_OBJ_TYPE, 432, 48, &File_023_ObjNum_022_48x208_6_19_26},
    {23, IMAGE_OBJ_TYPE, 432, 48, &File_024_ObjNum_023_48x208_6_19_26},
    {24, IMAGE_OBJ_TYPE, 432, 48, &File_025_ObjNum_024_48x208_6_19_26},
    {25, IMAGE_OBJ_TYPE, 432, 48, &File_026_ObjNum_025_48x208_6_19_26},
    {26, IMAGE_OBJ_TYPE, 432, 48, &File_027_ObjNum_026_48x208_6_19_26},
    {27, IMAGE_OBJ_TYPE, 432, 48, &File_028_ObjNum_027_48x208_6_19_26},
    {28, IMAGE_OBJ_TYPE, 432, 48, &File_029_ObjNum_028_48x208_6_19_26},
    {29, IMAGE_OBJ_TYPE, 408, 48, &File_030_ObjNum_029_64x25_6_19_26},
    {30, IMAGE_OBJ_TYPE, 408, 48, &File_031_ObjNum_030_64x25_6_19_26},
    {31, IMAGE_OBJ_TYPE, 0, 56, &File_032_ObjNum_031_96x190_6_19_26},
    {32, IMAGE_OBJ_TYPE, 0, 56, &File_033_ObjNum_032_96x190_6_19_26},
    {33, IMAGE_OBJ_TYPE, 0, 56, &File_034_ObjNum_033_96x190_6_19_26},
    {34, IMAGE_OBJ_TYPE, 0, 56, &File_035_ObjNum_034_96x190_6_19_26},
    {35, IMAGE_OBJ_TYPE, 0, 56, &File_036_ObjNum_035_96x190_6_19_26},
    {36, IMAGE_OBJ_TYPE, 0, 56, &File_037_ObjNum_036_96x190_6_19_26},
    {37, IMAGE_OBJ_TYPE, 0, 56, &File_038_ObjNum_037_96x190_6_19_26},
    {38, IMAGE_OBJ_TYPE, 0, 56, &File_039_ObjNum_038_96x190_6_19_26},
    {39, IMAGE_OBJ_TYPE, 0, 56, &File_040_ObjNum_039_96x190_6_19_26},
    {40, IMAGE_OBJ_TYPE, 0, 56, &File_041_ObjNum_040_96x190_6_19_26},
    {41, IMAGE_OBJ_TYPE, 0, 56, &File_042_ObjNum_041_96x190_6_19_26},
    {42, IMAGE_OBJ_TYPE, 96, 48, &File_043_ObjNum_042_144x208_6_19_26},
    {43, IMAGE_OBJ_TYPE, 96, 48, &File_044_ObjNum_043_144x208_6_19_26},
    {44, IMAGE_OBJ_TYPE, 96, 48, &File_045_ObjNum_044_144x208_6_19_26},
    {45, IMAGE_OBJ_TYPE, 96, 48, &File_046_ObjNum_045_144x208_6_19_26},
    {46, IMAGE_OBJ_TYPE, 96, 48, &File_047_ObjNum_046_144x208_6_19_26},
    {47, IMAGE_OBJ_TYPE, 96, 48, &File_043_ObjNum_042_144x208_6_19_26},
    {48, IMAGE_OBJ_TYPE, 96, 48, &File_044_ObjNum_043_144x208_6_19_26},
    {49, IMAGE_OBJ_TYPE, 96, 48, &File_045_ObjNum_044_144x208_6_19_26},
    {50, IMAGE_OBJ_TYPE, 96, 48, &File_046_ObjNum_045_144x208_6_19_26},
    {51, IMAGE_OBJ_TYPE, 96, 48, &File_047_ObjNum_046_144x208_6_19_26},
    {52, IMAGE_OBJ_TYPE, 96, 48, &File_048_ObjNum_052_144x183_6_19_26},
    {53, IMAGE_OBJ_TYPE, 96, 48, &File_049_ObjNum_053_144x183_6_19_26},
    {54, IMAGE_OBJ_TYPE, 96, 48, &File_050_ObjNum_054_144x183_6_19_26},
    {55, IMAGE_OBJ_TYPE, 96, 48, &File_051_ObjNum_055_144x183_6_19_26},
    {56, IMAGE_OBJ_TYPE, 96, 48, &File_049_ObjNum_053_144x183_6_19_26},
    {57, IMAGE_OBJ_TYPE, 240, 48, &File_052_ObjNum_057_144x30_6_19_26},
    {
        58,
        TEXT_OBJ_TYPE,
        0,
        0,
    },
    {
        59,
        TEXT_OBJ_TYPE,
        240,
        0,
    },
    {
        60,
        TEXT_OBJ_TYPE,
        408,
        0,
    },
    {
        61,
        TEXT_OBJ_TYPE,
        0,
        280,
    },
    {
        62,
        TEXT_OBJ_TYPE,
        264,
        280,
    },
    {
        63,
        TEXT_OBJ_TYPE,
        0,
        0,
    },
    {
        64,
        TEXT_OBJ_TYPE,
        0,
        80,
    },
    {
        65,
        TEXT_OBJ_TYPE,
        0,
        160,
    },
    {
        66,
        TEXT_OBJ_TYPE,
        0,
        240,
    },
    {
        67,
        TEXT_OBJ_TYPE,
        48,
        60,
    },
    {
        68,
        TEXT_OBJ_TYPE,
        48,
        112,
    },
    {
        69,
        TEXT_OBJ_TYPE,
        48,
        164,
    },
    {
        70,
        TEXT_OBJ_TYPE,
        48,
        216,
    },
    {
        71,
        TEXT_OBJ_TYPE,
        48,
        268,
    },
    {
        72,
        TEXT_OBJ_TYPE,
        288,
        60,
    },
    {
        73,
        TEXT_OBJ_TYPE,
        288,
        112,
    },
    {
        74,
        TEXT_OBJ_TYPE,
        288,
        164,
    },
    {
        75,
        TEXT_OBJ_TYPE,
        288,
        216,
    },
    {
        76,
        TEXT_OBJ_TYPE,
        288,
        268,
    },
    {77, IMAGE_OBJ_TYPE, 0, 64, &File_053_ObjNum_077_48x30_6_19_26},
    {78, IMAGE_OBJ_TYPE, 0, 116, &File_053_ObjNum_077_48x30_6_19_26},
    {79, IMAGE_OBJ_TYPE, 0, 168, &File_053_ObjNum_077_48x30_6_19_26},
    {80, IMAGE_OBJ_TYPE, 0, 220, &File_053_ObjNum_077_48x30_6_19_26},
    {81, IMAGE_OBJ_TYPE, 0, 272, &File_053_ObjNum_077_48x30_6_19_26},
    {82, IMAGE_OBJ_TYPE, 240, 64, &File_053_ObjNum_077_48x30_6_19_26},
    {83, IMAGE_OBJ_TYPE, 240, 116, &File_053_ObjNum_077_48x30_6_19_26},
    {84, IMAGE_OBJ_TYPE, 240, 168, &File_053_ObjNum_077_48x30_6_19_26},
    {85, IMAGE_OBJ_TYPE, 240, 220, &File_053_ObjNum_077_48x30_6_19_26},
    {86, IMAGE_OBJ_TYPE, 240, 272, &File_053_ObjNum_077_48x30_6_19_26},
    {87, IMAGE_OBJ_TYPE, 0, 48, &File_054_ObjNum_087_48x255_6_19_26},
    {88, IMAGE_OBJ_TYPE, 240, 48, &File_054_ObjNum_087_48x255_6_19_26},
    {
        89,
        TEXT_OBJ_TYPE,
        0,
        240,
    },
    {90, IMAGE_OBJ_TYPE, 0, 0, &File_055_ObjNum_090_480x320_6_18_26},
    {91, IMAGE_OBJ_TYPE, 0, 0, &File_056_ObjNum_091_480x320_6_18_26},
    {92, IMAGE_OBJ_TYPE, 0, 0, &File_057_ObjNum_092_480x320_6_18_26},
    {93, IMAGE_OBJ_TYPE, 0, 0, &File_058_ObjNum_093_480x320_6_18_26},
    {94, IMAGE_OBJ_TYPE, 0, 0, &File_059_ObjNum_094_480x320_6_18_26},
    {
        95,
        GROUPTABLE_OBJ_TYPE,
        0,
        0,
    },
    {
        96,
        GROUPTABLE_OBJ_TYPE,
        0,
        0,
    },
    {97, IMAGE_OBJ_TYPE, 0, 244, &File_062_ObjNum_097_480x40_6_19_26},
    {98, IMAGE_OBJ_TYPE, 240, 200, &File_063_ObjNum_098_176x33_6_19_26},
    {99, IMAGE_OBJ_TYPE, 264, 232, &File_064_ObjNum_099_48x63_6_19_26},
    {100, IMAGE_OBJ_TYPE, 288, 232, &File_064_ObjNum_099_48x63_6_19_26},
    {101, IMAGE_OBJ_TYPE, 312, 232, &File_064_ObjNum_099_48x63_6_19_26},
    {102, IMAGE_OBJ_TYPE, 336, 232, &File_064_ObjNum_099_48x63_6_19_26},
    {103, IMAGE_OBJ_TYPE, 360, 232, &File_064_ObjNum_099_48x63_6_19_26},
    {104, IMAGE_OBJ_TYPE, 264, 232, &File_065_ObjNum_104_16x30_6_19_26},
    {105, IMAGE_OBJ_TYPE, 288, 232, &File_065_ObjNum_104_16x30_6_19_26},
    {106, IMAGE_OBJ_TYPE, 312, 232, &File_065_ObjNum_104_16x30_6_19_26},
    {107, IMAGE_OBJ_TYPE, 336, 232, &File_065_ObjNum_104_16x30_6_19_26},
    {108, IMAGE_OBJ_TYPE, 360, 232, &File_065_ObjNum_104_16x30_6_19_26},
    {109, IMAGE_OBJ_TYPE, 240, 116, &File_066_ObjNum_109_96x80_6_19_26},
    {110, IMAGE_OBJ_TYPE, 264, 116, &File_066_ObjNum_109_96x80_6_19_26},
    {111, IMAGE_OBJ_TYPE, 288, 116, &File_066_ObjNum_109_96x80_6_19_26},
    {112, IMAGE_OBJ_TYPE, 312, 116, &File_066_ObjNum_109_96x80_6_19_26},
    {113, IMAGE_OBJ_TYPE, 336, 116, &File_066_ObjNum_109_96x80_6_19_26},
    {114, IMAGE_OBJ_TYPE, 240, 92, &File_067_ObjNum_114_64x103_6_19_26},
    {115, IMAGE_OBJ_TYPE, 264, 92, &File_067_ObjNum_114_64x103_6_19_26},
    {116, IMAGE_OBJ_TYPE, 288, 92, &File_067_ObjNum_114_64x103_6_19_26},
    {117, IMAGE_OBJ_TYPE, 312, 92, &File_067_ObjNum_114_64x103_6_19_26},
    {118, IMAGE_OBJ_TYPE, 336, 92, &File_067_ObjNum_114_64x103_6_19_26},
    {119, IMAGE_OBJ_TYPE, 240, 92, &File_068_ObjNum_119_64x103_6_19_26},
    {120, IMAGE_OBJ_TYPE, 264, 92, &File_068_ObjNum_119_64x103_6_19_26},
    {121, IMAGE_OBJ_TYPE, 288, 92, &File_068_ObjNum_119_64x103_6_19_26},
    {122, IMAGE_OBJ_TYPE, 312, 92, &File_068_ObjNum_119_64x103_6_19_26},
    {123, IMAGE_OBJ_TYPE, 336, 92, &File_068_ObjNum_119_64x103_6_19_26},
    {124, IMAGE_OBJ_TYPE, 240, 92, &File_069_ObjNum_124_64x103_6_19_26},
    {125, IMAGE_OBJ_TYPE, 264, 92, &File_069_ObjNum_124_64x103_6_19_26},
    {126, IMAGE_OBJ_TYPE, 288, 92, &File_069_ObjNum_124_64x103_6_19_26},
    {127, IMAGE_OBJ_TYPE, 312, 92, &File_069_ObjNum_124_64x103_6_19_26},
    {128, IMAGE_OBJ_TYPE, 336, 92, &File_069_ObjNum_124_64x103_6_19_26},
    {129, IMAGE_OBJ_TYPE, 240, 116, &File_070_ObjNum_129_96x80_6_19_26},
    {130, IMAGE_OBJ_TYPE, 264, 116, &File_070_ObjNum_129_96x80_6_19_26},
    {131, IMAGE_OBJ_TYPE, 288, 116, &File_070_ObjNum_129_96x80_6_19_26},
    {132, IMAGE_OBJ_TYPE, 312, 116, &File_070_ObjNum_129_96x80_6_19_26},
    {133, IMAGE_OBJ_TYPE, 336, 116, &File_070_ObjNum_129_96x80_6_19_26},
    {134, IMAGE_OBJ_TYPE, 240, 88, &File_071_ObjNum_134_176x188_6_19_26},
    {135, BACKGROUND_OBJ_TYPE, 0, 0, &File_072_ObjNum_135_480x320_6_18_26},
    {136, BACKGROUND_OBJ_TYPE, 0, 0, &File_073_ObjNum_136_480x320_6_18_26},
    {
        137,
        TEXT_OBJ_TYPE,
        144,
        232,
    },
    {138, IMAGE_OBJ_TYPE, 0, 168, &File_074_ObjNum_138_48x143_6_19_26},
    {139, IMAGE_OBJ_TYPE, 240, 168, &File_074_ObjNum_138_48x143_6_19_26},
    {140, IMAGE_OBJ_TYPE, 0, 176, &File_075_ObjNum_140_368x40_6_19_26},
    {141, IMAGE_OBJ_TYPE, 0, 224, &File_075_ObjNum_140_368x40_6_19_26},
    {142, IMAGE_OBJ_TYPE, 0, 272, &File_075_ObjNum_140_368x40_6_19_26},
    {143, IMAGE_OBJ_TYPE, 0, 176, &File_076_ObjNum_143_368x40_6_19_26},
    {144, IMAGE_OBJ_TYPE, 0, 224, &File_076_ObjNum_143_368x40_6_19_26},
    {145, IMAGE_OBJ_TYPE, 0, 272, &File_076_ObjNum_143_368x40_6_19_26},
    {
        146,
        TEXT_OBJ_TYPE,
        96,
        240,
    },
    {147, BACKGROUND_OBJ_TYPE, 0, 0, &File_077_ObjNum_147_480x320_6_18_26},
    {148, BACKGROUND_OBJ_TYPE, 0, 0, &File_078_ObjNum_148_480x320_6_18_26},
    {149, BACKGROUND_OBJ_TYPE, 0, 0, &File_079_ObjNum_149_480x320_6_17_26},
};
