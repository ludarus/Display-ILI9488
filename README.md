# Building
- Clone this repo
- Install STM32CubeMX
- Load project, then select [Display-ILI9488.ioc](Display-ILI9488.ioc)
- Generate code (generates build files)
- Load project onto MCU with debugger of choice

# Functionality
- Specialized driver for the Toyota Redmond CAN protocol
    - Hand coded, **NON-AGENTIC** development for highest possible code quality
    - MCU: STM32F091RC
    - Display controller: ILI9488 (480x320 resolution)
- Optimized image and text blitting
- Colour mode and monochrome mode
- Blit with OR mode or overwrite mode to combine with or replace the background
- Partial display updates
    - Write only the updated region to the screen
- SPI display connection at 12MBit baud
- CAN 2.0A network connection
    - Filter only relevant commands from protocol and display accordingly
    - Supports 128kb, 256kb, 500kb and 666.6kb baud rates
- Switch debouncing and broadcasting on CAN network
- PWM Alarm support 
    - Simple menu beeps
    - Togglable alarm with multiple frequencies
- UART serial logging
- Image compression and decompression to store in FLASH memory
- Gamma curved brightness adjustment

# Flow Details
- **Data Transmission - Monochrome Driver**
    - A clone of the screen buffer is bitpacked in memory and modified by the various text or image loading functions
    - This bitpacked data is then expanded to full colour resolution to a double buffer, which transmits one buffer via DMA while the other buffer is being expanded to for faster display times on the limited SPI bandwidth
    - This buffering approach is needed instead of storing the display data in full colour resolution due to memory limitations (only 32KiB of SRAM)
- **Data Transmission - Colour Driver**
    - The current background image is bitpacked in memory
    - On a blit call, the background info is expanded to full colour resolution to the double buffer and the text or image is expanded to a different colour on top of the background within the double buffer
    - This is because there is not enough memory to store additional colour data in the SRAM, so it must be done while transferring
- **General Notes**
    - Images are stored with specialized RLE compression in flash
    - The fontmap is bitpacked in flash
    - Pixels are stored LSB first
    - A lookup table is used to expand the bitpacked bytes into full colour resolution
    - Other tables are also used throughout the code. For more detail see the scripts section on how to generate customized tables

# Naming Conventions
- Types are named in PascalCase with a _t suffix to indiciate that it is a type
- Variables are named in camelCase, and if they have units there will be a _unit suffix, eg:
    - count_p for pixels
    - count_b for bytes
    - etc
- Static/inline functions are named in camelCase
- Driver functions are named in PascalCase with a ILI9488_ prefix
- Constants/macros are named in all caps CAMEL_CASE

# Specialized RLE Compression
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
- Use the [generate_lookup_table](scripts/generate_lookup_table.py) script to generate a lookup table to expand bitpacked bytes into full colour resolution from the specified on and off colours
```bash
$ python generate_lookup.py <on colour in R B G> <off colour in R B G>
```
# Object table generation
- Use the [generate_object_table](scripts/generate_object_table) script to generate the object table. All parameters are tweakable from within the file. 
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
