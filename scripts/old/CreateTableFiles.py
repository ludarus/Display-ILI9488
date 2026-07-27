# use this to compress the images contained in the .bin files.
# Files excluded from the compression are of ObjType 0x0d and 0x04 which are table types
# 0x0d - object table, 0x04 group tables.

import os
import glob

ObjList = []
i = 0
NumObjs = 0
# enumerate the fields in the object table for more readable access
Obj_Num = 0
ObjType = 1
xloc = 2
yloc = 3
xdim = 4
ydim = 5
DataType = 6
FlashLoc = 7
ObjSize = 8


def Table2Array():
    # read in the tables .bin from the original EL Raymond project
    # read the tables.bin file into an array. This is the original tables.bin from the EL Raymond project
    objnum1 = 0
    objtype1 = 0
    xloc1 = 0
    yloc1 = 0
    xdim1 = 0
    ydim1 = 0
    datatype1 = 0
    objsize1 = 0
    msb1 = 0
    lsb1 = 0
    i1 = 0

    try:
        in_file = open("tables/tableobj2.bin", "rb")  # open the table file
        filelen = os.path.getsize("tables/tableobj2.bin")
        NumObjs = filelen / 12  # 12 entries per row in the table object
        NumObjs = int(NumObjs)
        for i1 in range(NumObjs):  # create empty 2D array
            ObjList.append([])
            for j in range(10):  # 10 fields in the object table
                ObjList[i1].append([])
        i1 = 0
        while i1 < NumObjs:  # fill in the object table array
            lsb1 = in_file.read(1)
            msb1 = in_file.read(1)
            msb1 = msb1[0]
            lsb1 = lsb1[0]
            msb1 = msb1 * 256
            objnum1 = lsb1 + msb1
            objtype1 = in_file.read(1)
            objtype1 = objtype1[0]
            xloc1 = in_file.read(1)
            xloc1 = xloc1[0]
            yloc1 = in_file.read(1)
            yloc1 = yloc1[0]
            xdim1 = in_file.read(1)
            xdim1 = xdim1[0]
            ydim1 = in_file.read(1)
            ydim1 = ydim1[0]
            datatype1 = in_file.read(1)
            datatype1 = datatype1[0]
            lsb1 = in_file.read(1)
            msb1 = in_file.read(1)
            msb1 = msb1[0]
            lsb1 = lsb1[0]
            flashloc1 = lsb1 + (msb1 * 256)
            lsb1 = in_file.read(1)
            msb1 = in_file.read(1)
            msb1 = msb1[0]
            lsb1 = lsb1[0]
            objsize1 = lsb1 + (msb1 * 256)
            ObjList[i1] = [
                objnum1,
                objtype1,
                xloc1,
                yloc1,
                xdim1,
                ydim1,
                datatype1,
                flashloc1,
                objsize1,
            ]
            #            ObjList.append(ObjNum)
            i1 = i1 + 1
        in_file.close()
        print(ObjList)
        return 1

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

temp = Table2Array()
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

Tbl_file = open("Compressed/Tables/TablesObjNew3.txt", "w+")

Str1 = "\n\nX_DimTbl:\n\tdb\t"
for i in range(
    0, len(ObjList)
):  # loop through each object, skip object 0, the objtbl object.
    TempStr = "{:03x}".format(ObjList[i][xdim]) + "H"
    if i == 0:  # object 0 table object will be on it's own row'
        Str1 = Str1 + "000H\t ; 0 object not used.\n\tdb\t"
    elif i % 8 != 0:  # put a comma between values except at the end of line.
        Str1 = Str1 + TempStr + ",\t"
    elif i % 8 == 0:
        Str1 = Str1 + TempStr + "\t ; " + str(i - 7) + " - " + str(i) + "\n\tdb\t"
TempStr = Str1[-2:]
print(TempStr)
if TempStr == ",\t":
    Str1 = Str1[:-2]  # remove comma at end of file
    Str1 = Str1 + "\t ; " + str(i + 1 - (i % 8)) + " - " + str(i)
TempStr = Str1[-4:]
print(TempStr)
if TempStr == "\tdb\t":  # if we started a new row but had no values remove the db
    Str1 = Str1[:-4]
Tbl_file.write(Str1)

#  ********************************************************************
#           Y_DimTbl
#  ********************************************************************
Str1 = "\n\nY_DimTbl:\n\tdb\t"
for i in range(
    0, len(ObjList)
):  # loop through each object, skip object 0, the objtbl object.
    TempStr = "{:03x}".format(ObjList[i][ydim]) + "H"
    if i == 0:  # object 0 table object will be on it's own row'
        Str1 = Str1 + "000H\t ; 0 object not used.\n\tdb\t"
    elif i % 8 != 0:  # put a comma between values except at the end of line.
        Str1 = Str1 + TempStr + ",\t"
    elif i % 8 == 0:
        Str1 = Str1 + TempStr + "\t ; " + str(i - 7) + " - " + str(i) + "\n\tdb\t"
TempStr = Str1[-2:]
print(TempStr)
if TempStr == ",\t":
    Str1 = Str1[:-2]  # remove comma at end of file
    Str1 = Str1 + "\t ; " + str(i + 1 - (i % 8)) + " - " + str(i)
TempStr = Str1[-4:]
print(TempStr)
if TempStr == "\tdb\t":  # if we started a new row but had no values remove the db
    Str1 = Str1[:-4]
Tbl_file.write(Str1)


#  ********************************************************************
#           DataTypTbl
#  ********************************************************************

Str1 = "\n\nDataTypTbl:\n\tdb\t"
for i in range(
    0, len(ObjList)
):  # loop through each object, skip object 0, the objtbl object.
    TempStr = "{:03x}".format(ObjList[i][DataType]) + "H"
    if i == 0:  # object 0 table object will be on it's own row'
        Str1 = Str1 + "000H\t ; 0 object not used.\n\tdb\t"
    elif i % 8 != 0:  # put a comma between values except at the end of line.
        Str1 = Str1 + TempStr + ",\t"
    elif i % 8 == 0:
        Str1 = Str1 + TempStr + "\t ; " + str(i - 7) + " - " + str(i) + "\n\tdb\t"
TempStr = Str1[-2:]
print(TempStr)
if TempStr == ",\t":
    Str1 = Str1[:-2]  # remove comma at end of file
    Str1 = Str1 + "\t ; " + str(i + 1 - (i % 8)) + " - " + str(i)
TempStr = Str1[-4:]
print(TempStr)
if TempStr == "\tdb\t":  # if we started a new row but had no values remove the db
    Str1 = Str1[:-4]
Tbl_file.write(Str1)

#  ********************************************************************
#           FlashLocTbl
#  Table for storing the offset into the image database
#  This supports two byte database up to 64K in size
#  Values are stored LSB - MSB
#  ********************************************************************
Str1 = "\n\nFlashLocTbl:\t\t; LSB - MSB\n\tdb\t"
for i in range(
    0, len(ObjList)
):  # loop through each object, skip object 0, the objtbl object.
    TempStr = "{:04x}".format(ObjList[i][FlashLoc])
    LSB = TempStr[2:4]
    MSB = TempStr[0:2]
    if i == 0:  # object 0 table object will be on it's own row'
        Str1 = Str1 + "000H,\t000H\t; 0 object not used.\n\tdb\t"
    elif i % 4 != 0:  # put a comma between values except at the end of line.
        Str1 = Str1 + "0" + LSB + "H, 0" + MSB + "H,  "
    elif i % 4 == 0:
        Str1 = (
            Str1
            + "0"
            + LSB
            + "H, 0"
            + MSB
            + "H"
            + "\t ; "
            + str(i - 3)
            + " - "
            + str(i)
            + "\n\tdb\t"
        )
TempStr = Str1[-3:]
print(TempStr)
if TempStr == ",  ":
    Str1 = Str1[:-3]  # remove comma at end of file
    Str1 = Str1 + "\t ; " + str(i + 1 - (i % 4)) + " - " + str(i)
TempStr = Str1[-4:]
print(TempStr)
if TempStr == "\tdb\t":  # if we started a new row but had no values remove the db
    Str1 = Str1[:-4]
Tbl_file.write(Str1)


#  ********************************************************************
#           ObjSizeTbl
#  Table for storing the size of each object
#  This supports two byte
#  Values are stored LSB - MSB
#  ********************************************************************

Str1 = "\n\nObjSizeTbl:\t\t; LSB - MSB\n\tdb\t"
for i in range(
    0, len(ObjList)
):  # loop through each object, skip object 0, the objtbl object.
    TempStr = "{:04x}".format(ObjList[i][ObjSize])
    LSB = TempStr[2:4]
    MSB = TempStr[0:2]
    if i == 0:  # object 0 table object will be on it's own row'
        Str1 = Str1 + "000H,\t000H\t; 0 object not used.\n\tdb\t"
    elif i % 4 != 0:  # put a comma between values except at the end of line.
        Str1 = Str1 + "0" + LSB + "H, 0" + MSB + "H,  "
    elif i % 4 == 0:
        Str1 = (
            Str1
            + "0"
            + LSB
            + "H, 0"
            + MSB
            + "H"
            + "\t ; "
            + str(i - 3)
            + " - "
            + str(i)
            + "\n\tdb\t"
        )
TempStr = Str1[-3:]
print(TempStr)
if TempStr == ",  ":
    Str1 = Str1[:-3]  # remove comma at end of file
    Str1 = Str1 + "\t ; " + str(i + 1 - (i % 4)) + " - " + str(i)
TempStr = Str1[-4:]
print(TempStr)
if TempStr == "\tdb\t":  # if we started a new row but had no values remove the db
    Str1 = Str1[:-4]
Tbl_file.write(Str1)

#  ********************************************************************
#           RamLocTbl
#  Table for storing the location of the image in RamScreen
#  *** The location of RamScreen must match the raymond firmware ***
#  Ram location = Yloc * MaxXDim + Xloc + RamScreen
#  This supports two byte
#  Values are stored LSB - MSB
#  ********************************************************************
MaxXDim = 240 / 8  # screen width in bytes
RamScreen = 256  # *** this must match the firmware ***
Str1 = "\n\nRamLocTbl:\n\tdb\t"
for i in range(
    0, len(ObjList)
):  # loop through each object, skip object 0, the objtbl object.
    RamAddr = (ObjList[i][yloc] * MaxXDim) + ObjList[i][xloc] + RamScreen
    if (
        i == 107
    ):  # these images needed some location adjustments else the rounding made them overlap
        TempStr = "0BFC"
    elif i == 108:
        TempStr = "0BFE"
    elif i == 102:
        TempStr = "0BFC"
    elif i == 103:
        TempStr = "0BFE"
    else:
        TempStr = "{:04x}".format(int(RamAddr))
    LSB = TempStr[2:4]
    MSB = TempStr[0:2]
    if i == 0:  # object 0 table object will be on it's own row'
        Str1 = Str1 + "000H,\t000H\t; 0 object not used.\n\tdb\t"
    elif i % 4 != 0:  # put a comma between values except at the end of line.
        Str1 = Str1 + "0" + LSB + "H, 0" + MSB + "H,  "
    elif i % 4 == 0:
        Str1 = (
            Str1
            + "0"
            + LSB
            + "H, 0"
            + MSB
            + "H"
            + "\t ; "
            + str(i - 3)
            + " - "
            + str(i)
            + "\n\tdb\t"
        )
TempStr = Str1[-3:]
print(TempStr)
if TempStr == ",  ":
    Str1 = Str1[:-3]  # remove comma at end of file
    Str1 = Str1 + "\t ; " + str(i + 1 - (i % 4)) + " - " + str(i)
TempStr = Str1[-4:]
print(TempStr)
if TempStr == "\tdb\t":  # if we started a new row but had no values remove the db
    Str1 = Str1[:-4]
Tbl_file.write(Str1)
Tbl_file.close()
