# generates a lookup table to convert bitpacked bytes of pixels to expanded 3bpp coloured pixels

# generate_lookup_table.py
# Luke Fadel 2026

# Script to generate a lookup table to expand monochrome bitpacked bytes into full colour resolution
# 3 bits of colour per pixel: R G B
# Expanded format in one byte: 00RGBRGB
#   - fits two expanded pixels per byte
# Input specification:
#   - On and off colour = (R << 2) + (B << 1) + G, where R G and B are bits

import argparse


def main():
    # for command line arguments
    parser = argparse.ArgumentParser()
    # required arguments
    parser.add_argument(
        "onColour",
        type=int,
        help="Colour of the on pixels in integer form, = (R << 2) + (B << 1) + G where R G and B are bits",
    )
    parser.add_argument(
        "offColour",
        type=int,
        help="Colour of the off pixels in integer form, = (R << 2) + (B << 1) + G where R G and B are bits",
    )

    # parsing
    args = parser.parse_args()
    ON = args.onColour
    OFF = args.offColour
    # input checking
    if args.onColour > 0b111 or args.onColour < 0:
        ON = 0b111
    if args.offColour > 0b111 or args.offColour < 0:
        OFF = 0b000
    # outputs a sequence of 256 uint32_t's that maps a bitpacked byte into it's expanded version with the specified colours above
    # as 1 packed byte = 4 expanded bytes, thus uint8_t index input -> uint32_t expanded output
    print("const uint32_t bgPixelTable[256] = {")
    for i in range(256):
        result = 0
        for shift in (0, 2, 4, 6):
            # reminder: expanded pixel format is 00RGBRGB
            lo = ON if (i >> (shift)) & 1 else OFF
            hi = ON if (i >> (shift + 1)) & 1 else OFF
            result = (result >> 8) | (((lo << 3) | hi) << 24)
        comma = "," if i < 255 else " "
        print(f"    0x{result:08X}{comma}")
    print("};")


if __name__ == "__main__":
    main()
