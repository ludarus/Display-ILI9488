# generates a lookup table to offset for the non linearity of screen brightness

# trying an inverse gamma curve:
import math

gamma = 1.7

min_in = 0
max_in = 39

min_out = 1
max_out = 255

lut = [
    round(min_out + (i / max_in) ** (gamma) * (max_out - min_out))
    for i in range(max_in + 1)
]


# printing in C array format:
carray: str = ["const uint8_t brightnessTable[] = {"]

for n in lut:
    carray.append(f"\t{n},")

carray.append("};")

print("\n".join(carray))
