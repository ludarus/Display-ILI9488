# use this to compress the images contained in the .bin files.
# Files excluded from the compression are of ObjType 0x0d and 0x04 which are table types
# 0x0d - object table, 0x04 group tables.

import os


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

    def __repr__(self):
        # return f"objNum={self.objNum}, type={self.objType}, xloc={self.xLocation}, yloc={self.yLocation}, xdim={self.xDimension}, ydim{self.yDimension}, dataType={self.dataType}, flashLocation={self.flashLocation}, objSize={self.objSize}"
        return f"{self.objNum}, {self.objType}, {self.xLocation}, {self.yLocation}, {self.xDimension}, {self.yDimension}, {self.dataType}, {self.flashLocation}, {self.objSize}"


pathFromRoot = "tables/tableobj2.bin"


def Table2Array() -> list[Object]:
    # read in the tables .bin from the original EL Raymond project
    # read the tables.bin file into an array. This is the original tables.bin from the EL Raymond project

    ObjList: Object = []

    try:
        in_file = open(pathFromRoot, "rb")  # open the table file
        filelen = os.path.getsize(pathFromRoot)
        NumObjs = (filelen + 11) // 12  # 12 entries per row in the table object

        for i in range(NumObjs):  # fill in the object table array
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
                )
            )
        in_file.close()
        return ObjList

    except IOError:
        in_file.close()
        print("** Error While Opening the TableObj.bin!")
        quit()


#  Object Table format'
#  1 = LSB ObjNum'
#  2 = MSB ObjNum'
#  3 = ObjTypeTbl'
#  4 = X_LocTbl'
#  5 = Y_LocTbl'
#  6 = X_DimTbl'
#  7 = Y_DimTbl'
#  8 = DataTypTbl'
#  9 = FlashLocTbl LSB'
#  10 = FlashLocTbl MSB'
#  11 = ObjSizeTbl LSB'
#  12 = ObjSizeTbl MSB'
#  ObjList
# enumerate the fields in the object table for more readable access
#  Obj_Num = 0
#  ObjType = 1
#  xloc = 2
#  yloc = 3
#  xdim = 4
#  ydim = 5
#  DataType = 6
#  FlashLoc = 7
#  ObjSize = 8

#  ***********************************************************************************************'

print(Table2Array())
#  Tables to create
#  X_DimTbl	        ; 1 byte each X dimension in bytes
#  Y_DimTbl	        ; 1 byte each Y dimension in pixels
#  DataTypTbl		; 1 byte each data type
#  FlashLocTbl		; 2 bytes each offset in flash lsb,msb
#  ObjSizeTbl		; 2 bytes size of object it bytes
#  RamLocTbl		; 2 bytes of ram location
#  ********************************************************************
#           X_DimTbl
#  ********************************************************************
