# generates a lookup table to offset for the non linearity of screen brightness

# trying an inverse gamma curve:
import math

gamma = 2.2

min_in = 0
max_in = 255

min_out = 0
max_out = 255

lut = [
    round(min_out + (i / max_in) ** (gamma) * (max_out - min_out))
    for i in range(max_in + 1)
]


# printing in C array format:
carray: str = ["static const uint8_t brightnessTable[] = {"]

for n in lut:
    carray.append(f"\t{n},")

carray.append("};")

print("\n".join(carray))
