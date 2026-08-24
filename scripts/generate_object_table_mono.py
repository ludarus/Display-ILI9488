# use this to compress the images contained in the .bin files.
# Files excluded from the compression are of ObjType 0x0d and 0x04 which are table types
# 0x0d - object table, 0x04 group tables.

import os

# OBJECT TYPES:
# Type 13   = table object
# Type 0    = background object
# Type 3    = image object
# Type 4    = group table
# Type 1    = text
# Type 9    = flag? or something

# defining constants for scaling
OLD_WIDTH = 160

# original display = 160x80
OLD_HEIGHT = 80
# object 2 is the font map

NEW_WIDTH = 480
NEW_HEIGHT = 320

SCALING_WIDTH = NEW_WIDTH / OLD_WIDTH
SCALING_HEIGHT = NEW_HEIGHT / OLD_HEIGHT

backgroundIds = []
textIds = []
imageIds = []
groupIds = []


# object class for readability
class Object:
    # constructor
    def __init__(
        self,
        objNum: int,
        objType: int,
        xLocation: int,
        yLocation: int,
        xDimension: int,
        yDimension: int,
        dataType: int,
        flashLocation: int,
        objSize: int,
        imgReference: str,
        flashImgReference: str,
    ):
        self.objNum = objNum
        self.objType = objType
        self.xLocation = xLocation
        self.yLocation = yLocation
        self.xDimension = xDimension
        self.yDimension = yDimension
        self.dataType = dataType
        self.flashLocation = flashLocation
        self.objSize = objSize
        self.imgReference = imgReference
        self.flashImgReference = flashImgReference

    # print object override
    def __repr__(self):
        return f"""
objNum = {self.objNum},
type = {self.objType},
xloc = {self.xLocation},
yloc = {self.yLocation},
xdim = {self.xDimension},
ydim = {self.yDimension},
dataType = {self.dataType},
flashloc = {self.flashLocation},
objSize = {self.objSize}
imgRef = {self.imgReference}
flashImgRef= {self.flashImgReference}
"""


# project path for the binary input file
pathFromRoot = "tables/tableobj2.bin"

# global image list as a mapping from object number to image (if an image for it exists)
images: list[str] = [
    "&File_002_ObjNum_001_NEW_6_17_26",
    "",
    "",
    "&File_005_ObjNum_004_480x320_6_18_26",
    "&File_006_ObjNum_005_480x320_6_18_26_C",
    "&File_007_ObjNum_006_480x320_6_18_26",
    "&File_008_ObjNum_007_96x190_6_18_26",
    "&File_009_ObjNum_008_96x190_6_18_26",
    "&File_010_ObjNum_009_96x190_6_18_26",
    "&File_011_ObjNum_010_96x190_6_18_26",
    "&File_012_ObjNum_011_96x190_6_18_26",
    "&File_013_ObjNum_012_96x190_6_18_26",
    "&File_014_ObjNum_013_96x190_6_18_26",
    "&File_015_ObjNum_014_96x190_6_18_26",
    "&File_016_ObjNum_015_96x190_6_18_26",
    "&File_017_ObjNum_016_96x190_6_18_26",
    "&File_018_ObjNum_017_96x190_6_16_26",
    "&File_019_ObjNum_018_48x208_6_19_26",
    "&File_020_ObjNum_019_48x208_6_19_26",
    "&File_021_ObjNum_020_48x208_6_19_26",
    "&File_022_ObjNum_021_48x208_6_19_26",
    "&File_023_ObjNum_022_48x208_6_19_26",
    "&File_024_ObjNum_023_48x208_6_19_26",
    "&File_025_ObjNum_024_48x208_6_19_26",
    "&File_026_ObjNum_025_48x208_6_19_26",
    "&File_027_ObjNum_026_48x208_6_19_26",
    "&File_028_ObjNum_027_48x208_6_19_26",
    "&File_029_ObjNum_028_48x208_6_19_26",
    "&File_030_ObjNum_029_64x25_6_19_26",
    "&File_031_ObjNum_030_64x25_6_19_26",
    "&File_032_ObjNum_031_96x190_6_19_26",
    "&File_033_ObjNum_032_96x190_6_19_26",
    "&File_034_ObjNum_033_96x190_6_19_26",
    "&File_035_ObjNum_034_96x190_6_19_26",
    "&File_036_ObjNum_035_96x190_6_19_26",
    "&File_037_ObjNum_036_96x190_6_19_26",
    "&File_038_ObjNum_037_96x190_6_19_26",
    "&File_039_ObjNum_038_96x190_6_19_26",
    "&File_040_ObjNum_039_96x190_6_19_26",
    "&File_041_ObjNum_040_96x190_6_19_26",
    "&File_042_ObjNum_041_96x190_6_19_26",
    "&File_043_ObjNum_042_144x208_6_19_26",
    "&File_044_ObjNum_043_144x208_6_19_26",
    "&File_045_ObjNum_044_144x208_6_19_26",
    "&File_046_ObjNum_045_144x208_6_19_26",
    "&File_047_ObjNum_046_144x208_6_19_26",
    "",
    "",
    "",
    "",
    "",
    "&File_048_ObjNum_052_144x183_6_19_26",
    "&File_049_ObjNum_053_144x183_6_19_26",
    "&File_050_ObjNum_054_144x183_6_19_26",
    "&File_051_ObjNum_055_144x183_6_19_26",
    "",
    "&File_052_ObjNum_057_144x30_6_19_26",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "&File_053_ObjNum_077_48x30_6_19_26",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "&File_054_ObjNum_087_48x255_6_19_26",
    "",
    "",
    "&File_055_ObjNum_090_480x320_6_18_26",
    "&File_056_ObjNum_091_480x320_6_18_26",
    "&File_057_ObjNum_092_480x320_6_18_26",
    "&File_058_ObjNum_093_480x320_6_18_26",
    "&File_059_ObjNum_094_480x320_6_18_26",
    "",
    "",
    "&File_062_ObjNum_097_480x40_6_19_26",
    "&File_063_ObjNum_098_176x33_6_19_26",
    "&File_064_ObjNum_099_48x63_6_19_26",
    "",
    "",
    "",
    "",
    "&File_065_ObjNum_104_16x30_6_19_26",
    "",
    "",
    "",
    "",
    "&File_066_ObjNum_109_96x80_6_19_26",
    "",
    "",
    "",
    "",
    "&File_067_ObjNum_114_64x103_6_19_26",
    "",
    "",
    "",
    "",
    "&File_068_ObjNum_119_64x103_6_19_26",
    "",
    "",
    "",
    "",
    "&File_069_ObjNum_124_64x103_6_19_26",
    "",
    "",
    "",
    "",
    "&File_070_ObjNum_129_96x80_6_19_26",
    "",
    "",
    "",
    "",
    "&File_071_ObjNum_134_176x188_6_19_26",
    "&File_072_ObjNum_135_480x320_6_18_26",
    "&File_073_ObjNum_136_480x320_6_18_26",
    "",
    "&File_074_ObjNum_138_48x143_6_19_26",
    "",
    "&File_075_ObjNum_140_368x40_6_19_26",
    "",
    "",
    "&File_076_ObjNum_143_368x40_6_19_26",
    "",
    "",
    "",
    "&File_077_ObjNum_147_480x320_6_18_26",
    "&File_078_ObjNum_148_480x320_6_18_26",
    "&File_079_ObjNum_149_480x320_6_17_26",
]

# map of flash location to the first object that is stored there
flashMap = {}


def table2Array() -> list[Object]:
    """read in the tables .bin from the original EL Raymond project
    read the tables.bin file into an array."""

    ObjList: Object = []

    try:
        # open the table file
        in_file = open(pathFromRoot, "rb")

        filelen = os.path.getsize(pathFromRoot)

        # 12 entries per row in the table object
        NumObjs = (filelen + 11) // 12

        # fill in the object table array
        for i in range(NumObjs):
            lsb = in_file.read(1)
            msb = in_file.read(1)
            msb = msb[0]
            lsb = lsb[0]
            msb = msb * 256
            objNum = lsb + msb
            objType = in_file.read(1)
            objType = objType[0]
            xLoc = in_file.read(1)
            xLoc = xLoc[0]
            yLoc = in_file.read(1)
            yLoc = yLoc[0]
            xDim = in_file.read(1)
            xDim = xDim[0]
            yDim = in_file.read(1)
            yDim = yDim[0]
            datatype = in_file.read(1)
            datatype = datatype[0]
            lsb = in_file.read(1)
            msb = in_file.read(1)
            msb = msb[0]
            lsb = lsb[0]
            flashloc = lsb + (msb * 256)

            # assigning current object to flash location
            if flashMap.get(flashloc) is None and objType != 13:
                flashMap[flashloc] = objNum

            lsb = in_file.read(1)
            msb = in_file.read(1)
            msb = msb[0]
            lsb = lsb[0]
            objsize = lsb + (msb * 256)

            ObjList.append(
                Object(
                    objNum,
                    objType,
                    xLoc,
                    yLoc,
                    xDim,
                    yDim,
                    datatype,
                    flashloc,
                    objsize,
                    "" if objNum == 0 else images[objNum - 1],
                    (
                        ""
                        if flashMap.get(flashloc) is None or objNum == 0
                        else images[flashMap.get(flashloc) - 1]
                    ),
                )
            )

        in_file.close()
        return ObjList

    except IOError:
        in_file.close()
        print("** Error While Opening the TableObj.bin!")
        quit()


def convertToNewScreen(objList: list[Object]) -> list[Object]:
    """converts the old dimensions and coordinates to the scaled versions, and populates specialized arrays"""

    for obj in objList:
        # different conversion methods depending on the type
        match obj.objType:
            # table
            case 13:
                # applying enum
                obj.objType = "TABLE_OBJ_TYPE"

            # background
            case 0:
                obj.objType = "BACKGROUND_OBJ_TYPE"
                backgroundIds.append(obj.objNum)

                obj.xLocation = 0
                obj.yLocation = 0
                obj.xDimension = NEW_WIDTH
                obj.yDimension = NEW_HEIGHT
                obj.size = NEW_WIDTH * NEW_HEIGHT

            # image
            case 3:
                obj.objType = "IMAGE_OBJ_TYPE"
                imageIds.append(obj.objNum)

                obj.xLocation *= 8 * SCALING_WIDTH
                obj.yLocation *= SCALING_HEIGHT
                obj.xDimension *= 8 * SCALING_WIDTH
                obj.yDimension *= SCALING_HEIGHT

            # group table
            case 4:
                obj.objType = "GROUPTABLE_OBJ_TYPE"
                groupIds.append(obj.objNum)

            # text
            case 1:
                obj.objType = "TEXT_OBJ_TYPE"

                obj.xLocation *= 8 * SCALING_WIDTH
                # Text seems to be slightly out of bounds on y, so scaling this back a little bit
                obj.yLocation *= SCALING_HEIGHT

                # chopped off pixels from the font bitmap
                obj.yLocation += 4

                obj.xLocation = int(obj.xLocation)
                # tuple
                textIds.append((obj.objNum, obj.xLocation))

            # ???
            case 9:
                obj.objType = "UNKNOWN_OBJ_TYPE"

        # casting back to integer
        obj.xLocation = int(obj.xLocation)
        obj.yLocation = int(obj.yLocation)
        obj.xDimension = int(obj.xDimension)
        obj.yDimension = int(obj.yDimension)

        # out of bounds check - only triggers for object 2 which is special for some reason
        if (
            obj.xLocation + obj.xDimension > NEW_WIDTH
            or obj.yLocation + obj.yDimension > NEW_HEIGHT
        ):
            print(obj)

    return objList


def generateArray(modifiedObjlist: list[Object]) -> list[str]:
    """generates a c array from the object list"""

    # removing the first entry
    modifiedObjlist.pop(0)

    # generates a c array from the modified objlist
    lines: list[str] = ["const Obj_t objects[] = {"]

    for object in modifiedObjlist:
        lines.append(
            "\t{"
            + f"{object.objNum}, {object.objType}, {object.xLocation}, {object.yLocation}, {object.flashImgReference}"
            + "},"
        )

    lines.append("};")

    return "\n".join(lines)


#  ***********************************************************************************************

baseList = table2Array()
modifiedList = convertToNewScreen(baseList)
cArray = generateArray(modifiedList)

print(modifiedList)
print(cArray)

print("bgs: \n", tuple(backgroundIds))
print("images: \n", tuple(imageIds))
print("groups: \n", tuple(groupIds))
print("text: \n", tuple(textIds))
