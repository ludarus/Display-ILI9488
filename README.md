# Building
- Clone this repo
- Install STM32CubeMX
- Load project, then select [Display-ILI9488.ioc](Display-ILI9488.ioc)
- Generate code (generates build files)
- Load project onto MCU with debugger of choice

# Functionality
- Image and text displaying
    - Blit with OR mode or overwrite mode to either replace the current display section or draw on top of it
- Variable image location
    - Off screen drawing is supported
- 1 bit per pixel resolution
- Partial display updates
    - Write only the updated region of the screen to the display controller
- Serial connection 
    - Receive, process and send diagnostics via USART serial connection
- CAN connection
    - Receive and process CAN 2.0A commands

# Documentation
- Pixels are bitpacked to save memory
- Pixels are stored LSB first
- A lookup table is used for fast byte unpacking
    - Expansion to 3 bit per pixel colour resolution, plus an additional pixel of padding
- RLE is used to compress the images
- Bitpacking is used to compress the font characters
- Chunking and a dual buffer is used to transmit the image data via the SPI connection

# RLE Compression
- Sequence begins with number of OFF bits in a row, then alternates between contiguous ON and OFF bits in a row in this form:
    - **OFF, ON, OFF, ON**
- Examples
    - **7, 9, 3, 4, 1**
    represents **7** off pixels, **9** on pixels, **3** off pixels, **4** on pixels, and **1** off pixel
    - **0, 4, 8, 2** 
    represents **4** on pixels, **8** off pixels, **2** on pixels
- If there are more than **255** of the same pixel in a row:
    - Insert **255, 0,** then continue with the rest of the number (n - 255)
- Example
    - **0, 600, 3** becomes 
     **0, 255, 0, 255, 0, 90, 3**
- This allows decoding based only on the divisibility by 2 of the index of any term in the compressed sequence 

# BMP conversion
- Use the [BMP_parser](scripts/BMP_parser.py) script to compress a 1bpp BMP file to a c header file.
- The output directory should be [Core/Inc/](Core/Inc)

**Single file usage**
```bash
$ python BMP_parser.py <input file> <output directory>
```
**Full directory usage**
```bash
$ python BMP_parser.py <input file> <output directory> -d
```

**Font usage**
```bash
$ python BMP_parser.py <input file> <output directory> -f -cw <character width in px>
```

# Lookup table generation
- Use the [generate_lookup_table](scripts/generate_lookup_table.py) script to generate a lookup table from the specified on and off colours
```bash
$ python generate_lookup.py <on colour in R B G> <off colour in R B G>
```
# Object table generation
- Use the [modified_CreateTableFiles](scripts/modified_CreateTableFiles.py) script to generate the object table. All parameters are tweakable from within the file. 
```bash
$ python modified_CreateTableFiles.py
```

# Brightness table generation
- Use the [generate_brightness_table](scripts/generate_brightness_table.py) script to generate a lookup brightness table. Gamma parameters can be tweaked from within the file.
```bash
$ python generate_brightness_table.py
```

# CAN Network emulation
- Use the [TRC_parser](scripts/TRC_parser.py) script to emulate an inputted .trc log file. A USB to CAN adapter is required.
```bash
$ python TRC_parser.py <input .trc file>
```

# Stress testing
- Use the [stress_test_generator](scripts/stress_test_generator.py) script to send test signals to the MCU. A USB to CAN adapter is required.
```bash
$ python stress_test_generator.py 
```
