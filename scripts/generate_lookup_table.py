# generates a lookup table to convert bitpacked bytes of pixels to expanded 4bpp coloured pixels

import argparse


def main():
    # for command line arguments
    parser = argparse.ArgumentParser()

    # required arguments
    parser.add_argument(
        "onColour",
        type=int,
        help="Colour of the on pixels in integer form. The 3 bit format is R G B.",
    )
    parser.add_argument(
        "offColour",
        type=int,
        help="Colour of the off pixels in integer form. The 3 bit format is R G B.",
    )

    # parsing
    args = parser.parse_args()

    ON = args.onColour
    OFF = args.offColour

    # input checking
    if args.onColour > 0b1110 or args.onColour < 0:
        ON = 0b1110
    if args.offColour > 0b1110 or args.offColour < 0:
        OFF = 0b1110

    # outputs a sequence of 256 uint32_t's that maps a bitpacked byte into it's expanded version with the specified colours above
    print("static const uint32_t pixelTable[256] = {")

    for i in range(256):
        result = 0
        for shift in (0, 2, 4, 6):
            lo = ON if (i >> (shift)) & 1 else OFF
            hi = ON if (i >> (shift + 1)) & 1 else OFF
            result = (result >> 8) | (((lo << 4) | hi) << 24)

        comma = "," if i < 255 else " "
        print(f"    0x{result:08X}{comma}")

    print("};")


if __name__ == "__main__":
    main()
