# will parse a table and output correct coordinates
import argparse
import os
import sys
import math

OLD_WIDTH = 240
OLD_HEIGHT = 128

NEW_WIDTH = 480
NEW_HEIGHT = 320


# to easily change the decoding method of the bytes for testing
def hex_to_index(firstByte: str, secondByte: str) -> int:
    return int(firstByte, 16) + (int(secondByte, 16) << 8) - 256


def parse_table(tableFilePath: str) -> list[int]:
    with open(tableFilePath, "rt") as f:
        file = f.read()

    lines = file.splitlines()

    indicies: list[int] = []

    for line in lines:
        # isolating each hex number

        # first two are different than the rest
        hindex = line.find("H")
        firstByte = line[:hindex]
        line = line[hindex + 1 :]

        hindex = line.find("H")
        secondByte = line[2:hindex]
        line = line[hindex + 1 :]

        indicies.append(hex_to_index(firstByte, secondByte))

        while line.find("H") != -1:
            hindex = line.find("H")
            firstByte = line[3:hindex]
            line = line[hindex + 1 :]

            hindex = line.find("H")
            secondByte = line[2:hindex]
            line = line[hindex + 1 :]

            indicies.append(hex_to_index(firstByte, secondByte))

    return indicies


def convert_to_coords(indicies: list[int]) -> list[tuple[int]]:
    coords: list[tuple[int]] = []
    for num in indicies:
        # getting coordinates from global index
        x = num % OLD_WIDTH
        y = num // OLD_WIDTH

        # scaling and rounding to nearest 8
        # x = int(x * (NEW_WIDTH / OLD_WIDTH))  # // 8) * 8)
        # y = int(y * (NEW_HEIGHT / OLD_HEIGHT))

        coords.append((x, y))

    return coords


def modify_c_array(arrayFilePath: str, coords: list[tuple[int]]) -> list[str]:
    with open(arrayFilePath, "rt") as f:
        file = f.read()

    lines = file.splitlines()

    for i in range(len(coords)):
        line = lines[i + 1]

        # getting positions before and after the x and y values
        beforeX = line.find(" ", line.find("{"), len(line) - 1)
        afterX = line.find(",", beforeX, len(line) - 1)

        beforeY = line.find(" ", afterX, len(line) - 1)
        afterY = line.find(",", beforeY, len(line) - 1)

        # inserting new x yalue
        lines[i + 1] = line[: beforeY + 1] + str(coords[i][1]) + line[afterY:]

        line = lines[i + 1]

        # inserting new x value
        lines[i + 1] = line[: beforeX + 1] + str(coords[i][0]) + line[afterX:]

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("table", help="a table to parse")
    parser.add_argument("array", help="an array template to modify")

    args = parser.parse_args()

    # checking if input is a file
    if not os.path.isfile(args.table):
        print(f"ERROR: File not found: {args.table}", file=sys.stderr)
        sys.exit(1)

    if not os.path.isfile(args.array):
        print(f"ERROR: File not found: {args.array}", file=sys.stderr)
        sys.exit(1)

    print(modify_c_array(args.array, convert_to_coords(parse_table(args.table))))


if __name__ == "__main__":
    main()
