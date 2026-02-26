# gen_draw_table256_pretty.py
On  = "On"
Off = "Off"

def bits(byte):
    """Return list of On/Off and ASCII pattern string for bits 7→0."""
    mask = [1 << (7 - i) for i in range(8)]
    row  = [On if (byte & m) else Off for m in mask]
    art  = "".join("█" if (byte & m) else "." for m in mask)
    return row, art

print("static const uint32_t draw_table[256][8] = {")
for b in range(256):
    row, art = bits(b)
    joined = ", ".join(f"{x:>3}" for x in row)
    print(f"    {{ {joined} }},  // 0x{b:02X}  {art}")
print("};")
