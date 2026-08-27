# generate_brightness_table.py
# Luke Fadel 2026

# Script to generate a visually linear brightness table
# Tweak all parameters from within the script, no command line flags
# Paste output into Src/tables.c

import math

# gamma coefficient. tweaks the brightness curve of the output
gamma = 1.7

# range for table input. number of elements = max_in - min_in + 1
min_in = 0
max_in = 39

# range for output pwm values. max = 255 min = 0 for 8 bit int
min_out = 1
max_out = 255

# applying gamma function
lut = [
    round(min_out + (i / max_in) ** (gamma) * (max_out - min_out))
    for i in range(max_in + 1)
]


# printing in C array format:
carray: str = ["const uint8_t brightnessTable[] = {"]

for n in lut:
    # tabbing the list
    carray.append(f"\t{n},")

carray.append("};")

print("\n".join(carray))
