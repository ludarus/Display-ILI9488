#!/usr/bin/env python3

# BMP_parser.py
# Luke Fadel 2026

# This script converts 1bpp monochrome .bmp files into:
# - RLE compressed C header files for images
# - bitpack compressed C header file for fontmap
# Proper usage is detailed in the README, and running this script with a -h flag

# imports for commandline argument parsing, file access, etc
import argparse
import sys
import os
import re
import struct


# parses a 1bpp bmp file and outputs a list of the bits
def parse_file(path: str):
    """Parses a 1bpp BMP file"""

    # reading binary from file
    with open(path, "rb") as f:
        raw = f.read()

    # if first two bytes aren't BM - BMP header indication
    if raw[:2] != b"BM":
        raise ValueError("Not a BMP file")

    # unpacking data from header offset
    pixel_offset = struct.unpack_from("<I", raw, 10)[0]

    # DIB header
    dib_size = struct.unpack_from("<I", raw, 14)[0]

    # if header is too old
    if dib_size < 40:
        raise ValueError("unsupported dib header size")

    # extracting bmp variables
    width = struct.unpack_from("<i", raw, 18)[0]
    height = struct.unpack_from("<i", raw, 22)[0]
    planes = struct.unpack_from("<H", raw, 26)[0]
    bpp = struct.unpack_from("<H", raw, 28)[0]
    compression = struct.unpack_from("<I", raw, 30)[0]

    # checking if BMP is monochrome
    if bpp != 1:
        raise ValueError(f"expected 1bpp BMP, got {bpp}")

    if compression != 0:
        raise ValueError("compression BMPs are not supported")

    if planes != 1:
        raise ValueError(f"expected 1 colour plane, got {planes}")

    # if the height is negative, then the bitmap is top down
    top_down = height < 0
    height = abs(height)

    # calculating total length of row
    row_stride = ((width + 31) // 32) * 4

    # slicing off the padding
    pixel_data = raw[pixel_offset:]

    rows = []

    for r in range(height):
        # flipping the rows upside down for top to bottom filling
        # top down                      bottom up
        bmp_row_index = r if top_down else (height - 1 - r)
        row_start = bmp_row_index * row_stride
        # slicing the current row between the start and end
        # this still includes the up to 4 bytes of padding at the end
        row_bytes = pixel_data[row_start : row_start + row_stride]
        rows.append(row_bytes)

    output_bits = []

    # converting pixels to bytes
    bytes_per_row = (width + 7) // 8
    leftover = width % 8

    # converting byte array to an array of bits (1s and 0s)
    for row in rows:
        for byte_index in range(bytes_per_row):
            b = row[byte_index]
            if byte_index == bytes_per_row - 1 and leftover != 0:
                bits = [(b >> i) & 1 for i in range(leftover - 1, -1, -1)]
            else:
                bits = [(b >> i) & 1 for i in range(7, -1, -1)]
            output_bits.extend(bits)

    # debug print
    for y in range(height):
        for x in range(width):
            print("█" if output_bits[(y * width) + x] else ".", end="")
        print("")

    return width, height, output_bits


# turns a font map into an array of bitpacked character arrays
def bitpack_font(data: list, width: int, height: int, charWidth: int) -> list:
    """Turns a font map into an array of bitpacked character arrays"""

    # calculating the number of characters in the map
    amount_of_characters = width // charWidth

    # an array of bitpacked bmps for each character
    output = []

    # iterating through each image segment
    for charIdx in range(amount_of_characters):
        # per character bitpacked array
        character = []
        # iterating through the specific segment of the image (px wise)
        for localIdx in range((charWidth * height) // 8):

            # packing the current byte
            byte = 0b00000000
            for i in range(8):
                # converting local character index to map index
                globalpos = (
                    width * (((localIdx * 8) + i) // charWidth)
                    + (((localIdx * 8) + i) % charWidth)
                    + (charIdx * charWidth)
                )
                # LSB first
                byte |= data[globalpos] << i
            character.append(byte)
        output.append(character)
        # debug print statement
        print(character)

    return output


# bitpacks an image. Not used in production code as RLE had better average compression
def bitpack_image(data: list) -> list:
    """Bitpacks an image"""
    output = []

    # iterating through every byte in the bit array
    for byteIdx in range(len(data) // 8):
        byte = 0b00000000
        for i in range(8):
            # shifting current bit into position
            byte |= data[(byteIdx * 8) + i] << i
        output.append(byte)
    return output


# RLE compresses data
def compress_image_RLE(data: list) -> list:
    """Compress data with specialized run length encoding"""
    # compressing byte array to extreme RLE. Detailed on Github repo as well.
    # FORMAT:
    #     - Sequence begins with number of OFF bits in a row
    #     - Then alternates between contiguous ON and OFF bits in a row in this form:
    #     - OFF, ON, OFF, ON
    #     - example:
    #         7, 9, 3, 4, 1
    #         represents 7 off pixels, 9 on pixels, 3 off pixels etc
    #
    #         0, 4, 8, 2
    #         represents 4 on pixels, 8 off pixels, 2 on pixels
    # - IF there is more than 255 of the same pixel in a row,
    # - put 255, 0, then continue with the rest of the number (n - 255)
    #
    # - 0, 600, 3 becomes
    # - 0, 255, 0, 255, 0, 90, 3

    compressed_bits = []

    # adding initial 0 if the first section of bits is 1
    if data[0] == 1:
        compressed_bits.append(0)

    count = 1

    # iterating through every bit
    for bit_index in range(len(data) - 1):

        # checking if the current run is contiguous
        if data[bit_index] == data[bit_index + 1]:

            # if the 8 bit limit is reached, append 255, 0, and continue with the encoding
            if count == 255:
                compressed_bits.append(255)
                compressed_bits.append(0)
                count = 1
            # increment count
            else:
                count += 1

        # reset count if the two bits don't match
        else:
            compressed_bits.append(count)
            count = 1

    compressed_bits.append(count)

    return compressed_bits


# Turns a filename into a valid C identifier
def sanitise_name(name: str) -> str:
    """Turn a filename into a valid C identifier"""

    name = os.path.splitext(os.path.basename(name))[0]
    name = re.sub(r"[^a-zA-Z0-9_]", "_", name)
    if name and name[0].isdigit():
        name = "_" + name
    return name


def generate_image_header(
    data: list, var_name: str, width: int, height: int, rle: bool
):
    """Generates a C header for an inputted image"""

    guard = f"{var_name.upper()}_H"

    bytes_per_row = (width + 7) // 8
    total_bytes = len(data)

    lines = []
    # writing start of headaer file
    lines.append(f"#ifndef {guard}")
    lines.append(f"#define {guard}")
    lines.append("")
    lines.append("#include <stdint.h>")
    lines.append('#include "image.h"')
    lines.append("")
    lines.append("/* Auto-generated by BMP_parser.py")
    lines.append(f" * Image : {var_name}")
    lines.append(f" * Size  : {width} x {height} pixels")
    if rle:
        lines.append(" * Compression method : Run length encoding (see github page)")
    else:
        lines.append(" * Compression method : Bit packing (8 pixels per byte)")
    lines.append(
        f" * Layout: {bytes_per_row} byte(s)/row x {height} rows = {total_bytes} bytes total"
    )
    lines.append(" * Pixel 0,0 is top-left.")
    lines.append(" */")
    lines.append("")
    # writing image array
    lines.append(f"static const uint8_t {var_name}_DATA[{len(data)}U] = {{")
    for value in data:
        hex_str = f"0x{value:02X}"
        lines.append(f"     {hex_str},  /* {format(value, '08b')} */")
    lines.append("};")
    lines.append("")
    lines.append(f"static const Image_t {var_name} = " + "{")
    lines.append(f"     .width = {width},")
    lines.append(f"     .height = {height},")
    lines.append(f"     .size = {len(data)},")
    lines.append(f"     .data = {var_name}_DATA")
    lines.append("};")
    lines.append("")
    lines.append(f"#endif /* {guard} */")
    lines.append("")

    return "\n".join(lines)


def generate_font_header(
    data: list, var_name: str, characterWidth: int, characterHeight: int
):
    """Generates a C header file for a font set"""

    guard = f"{var_name.upper()}_H"
    lines = []
    lines.append(f"#ifndef {guard}")
    lines.append(f"#define {guard}")
    lines.append("")
    lines.append("#include <stdint.h>")
    lines.append('#include "character.h"')
    lines.append("")
    lines.append(f"#define CHARWIDTH {characterWidth}")
    lines.append(f"#define CHARHEIGHT {characterHeight}")
    lines.append(f"#define FONTSIZE {len(data)}")
    lines.append("")
    lines.append("// Auto-generated by BMP_parser.py")
    lines.append("")

    # generating data arrays
    for charIdx in range(len(data)):
        lines.append(
            f"static const uint8_t {var_name}_DATA{charIdx}[{len(data[charIdx])}] = "
            + "{"
        )
        for byte in data[charIdx]:
            lines.append(f"  {byte},")
        lines.append("};")
        lines.append("")

    # generating ascii map (an array in ASCII order with a 32 offset)
    lines.append(f"static const Character_t {var_name}[] = " + "{")
    for charIdx in range(len(data)):
        lines.append("  {")
        lines.append(f"  .asciiCode = {charIdx + 32},")
        lines.append(f"  .data = {var_name}_DATA{charIdx},")
        lines.append("  },")
    lines.append("};")

    lines.append("")
    lines.append(f"#endif /* {guard} */")
    lines.append("")

    return "\n".join(lines)


def main():
    # getting arguments from cli
    parser = argparse.ArgumentParser()

    # required arguments: file input and output
    parser.add_argument("input", help="a 1bpp BMP file")
    parser.add_argument("output", help="output directory of the header file")

    # optional arguments: directory mode, font mode & character width, bitpack mode, and preview mode
    parser.add_argument(
        "-d",
        "--directory",
        action="store_true",
        help="attempt to convert every file in the directory",
    )
    parser.add_argument(
        "-f",
        "--font",
        action="store_true",
        help="takes a monospaced 1bpp font map bmp as input and outputs a list of character headers. Must include -cw flag to specify character width in pixels",
    )
    parser.add_argument(
        "-cw",
        "--char-width",
        type=int,
        metavar="WIDTH",
        help="width of each character in the monospaced font (required with -f)",
    )
    parser.add_argument(
        # this argument is not used in the final code, only for testing
        "-bp",
        "--bitpack",
        action="store_true",
        help="NOTE: UNUSED IN FINAL CODE | use run length encoding instead of bit packing to compress the image",
    )
    parser.add_argument(
        "-p",
        "--preview",
        action="store_true",
        help="only display preview and don't convert to c header",
    )

    args = parser.parse_args()

    # Verifying the inputted files/directories exist
    if not os.path.isfile(args.input):
        print(f"ERROR: File not found: {args.input}", file=sys.stderr)
        sys.exit(1)

    if not os.path.isdir(args.output):
        print(f"ERROR: Directory not found: {args.output}", file=sys.stderr)
        sys.exit(1)
    if args.font and args.char_width is None:
        print("ERROR: character width (-cw) not provided for font flag (-f)")

    # Logic section:

    # normal mode
    if not args.directory:
        # attempting to parse and compress file
        try:
            # parsing bmp file
            width, height, uncompressed_data = parse_file(args.input)
            # setting variable name to file name
            var_name = sanitise_name(args.input)

            # font mode
            if args.font:
                packed_data = bitpack_font(
                    uncompressed_data, width, height, args.char_width
                )
                header = generate_font_header(
                    packed_data, var_name, args.char_width, height
                )

            # image mode
            else:
                compressed_data = (
                    compress_image_RLE(uncompressed_data)
                    if not args.bitpack
                    else bitpack_image(uncompressed_data)
                )
                header = generate_image_header(
                    compressed_data, var_name, width, height, not args.bitpack
                )

            # writing to files if not in preview mode
            if not args.preview:
                with open(f"{args.output}{var_name}.h", "w") as f:
                    f.write(header)
                print(
                    f"Successfully compressed bmp to c array. Header file is located in {args.output}{var_name}.h"
                )

        # On error
        except ValueError as e:
            print(f"ERROR: {e}", file=sys.stderr)
            sys.exit(1)

    # if in directory mode
    else:
        # attempting to convert every file in the directory
        try:
            count = 0
            dirName = os.path.dirname(args.input)

            # helpful include statement to include all the images
            includes = []

            # helpful array of pointers to each image for cycling and indexing each image
            images_array = []

            # total size
            total_size = 0

            # iterating through every file in the provided directory
            for possibleFile in os.listdir(dirName):
                if os.path.isfile(f"{dirName}/{possibleFile}"):

                    # attempting to parse the file as a BMP
                    width, height, uncompressed_data = parse_file(
                        f"{dirName}/{possibleFile}"
                    )

                    # assigning variable name as file name
                    var_name = sanitise_name(f"{dirName}/{possibleFile}")

                    # font mode
                    if args.font:
                        packed_data = bitpack_font(
                            uncompressed_data, width, height, args.char_width
                        )
                        header = generate_font_header(
                            packed_data, var_name, args.char_width, height
                        )

                    # image mode
                    else:
                        compressed_data = (
                            compress_image_RLE(uncompressed_data)
                            if not args.bitpack
                            else bitpack_image(uncompressed_data)
                        )
                        header = generate_image_header(
                            compressed_data, var_name, width, height, not args.bitpack
                        )

                    # individual file confirmation message
                    if not args.preview:
                        with open(f"{args.output}{var_name}.h", "w") as f:
                            f.write(header)

                        print(
                            f"Successfully compressed {dirName}/{possibleFile} to c array. Header file is located in {args.output}{var_name}.h"
                        )
                        includes.append(f'#include "{var_name}.h"')
                        images_array.append(f"   &{var_name},")
                        total_size += len(compressed_data)
                        count += 1
            # final output
            if not args.preview:
                images_array.insert(0, f"const Image_t *images[{count}] = " + "{")
                images_array.append("};")
                print(
                    f"successfully converted {count} files in {os.path.dirname(args.input)}"
                )
                # quality of life statements
                print("include statement: \n" + "\n".join(includes))
                print("images array: \n" + "\n".join(images_array))
                print(f"total size: {total_size}B")

        except ValueError as e:
            print(f"ERROR: {e}", file=sys.stderr)
            sys.exit(1)


if __name__ == "__main__":
    main()
