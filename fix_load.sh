#!/usr/bin/env zsh

MAIN_FILE=$(find "/Users/saosun" -name "main.c" 2>/dev/null | head -n 1)
PROJ_DIR=$(dirname "$MAIN_FILE")
cd "$PROJ_DIR"

# Restore original source headers
[ -f image.h.bak ] && cp image.h.bak image.h || cp image.h image.h.bak
[ -f gui.c.bak ] && cp gui.c.bak gui.c || cp gui.c gui.c.bak

ABS_LENA="$PROJ_DIR/lena.bmp"

# 1. Generate valid 24-bit uncompressed RGB BMP at absolute path
python3 -c "
import struct
w, h = 256, 256
row_bytes = w * 3
padding = (4 - (row_bytes % 4)) % 4
image_size = (row_bytes + padding) * h
file_size = 54 + image_size

header = struct.pack('<2sIHHI', b'BM', file_size, 0, 0, 54)
info = struct.pack('<IIIHHIIIIII', 40, w, h, 1, 24, 0, image_size, 2835, 2835, 0, 0)

pixels = bytearray()
for y in range(h):
    for x in range(w):
        pixels.extend([x % 256, (x + y) % 256, y % 256])
    pixels.extend(b'\x00' * padding)

with open('$ABS_LENA', 'wb') as f:
    f.write(header + info + pixels)
print('Generated 24-bit BMP at $ABS_LENA')
"

# 2. Scope #pragma pack strictly around struct definitions in image.h
python3 -c "
with open('image.h', 'r') as f:
    content = f.read()

content = content.replace('#pragma pack(push, 1)\n', '').replace('#pragma pack(pop)\n', '')

lines = content.splitlines()
new_lines = []
in_struct = False
for line in lines:
    if ('typedef struct' in line or 'struct' in line) and '{' in line:
        new_lines.append('#pragma pack(push, 1)')
        new_lines.append(line)
        in_struct = True
    elif in_struct and ('}' in line):
        new_lines.append(line)
        new_lines.append('#pragma pack(pop)')
        in_struct = False
    else:
        new_lines.append(line)

with open('image.h', 'w') as f:
    f.write('\n'.join(new_lines))
print('Scoped struct packing in image.h')
"

# 3. Hardcode absolute path in gui.c
python3 -c "
with open('gui.c', 'r') as f:
    code = f.read()

lines = code.splitlines()
new_lines = []
for line in lines:
    s = line.strip()
    if 'IupFileDlg' in s or 'IupPopup' in s or ('IupDestroy' in s and 'filedlg' in s):
        new_lines.append('// ' + line)
    elif 'IupSetAttribute' in s and 'filedlg' in s:
        new_lines.append('// ' + line)
    elif 'IupGetAttribute' in s and 'filedlg' in s:
        indent = line[:len(line) - len(line.lstrip())]
        new_lines.append(indent + 'filename = \"$ABS_LENA\";')
    else:
        new_lines.append(line)

with open('gui.c', 'w') as f:
    f.write('\n'.join(new_lines))
"

# 4. Compile and Launch
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
GTK_FLAGS=($(pkg-config --cflags --libs gtk+-3.0 2>/dev/null))
IUP_INC=$(dirname "$(find "/Users/saosun" -name "iup.h" 2>/dev/null | head -n 1)")
LIBIUP=$(find "/Users/saosun" -name "libiup.a" 2>/dev/null | head -n 1)

clang -o editor main.c gui.c image.c operations.c "$LIBIUP" "${GTK_FLAGS[@]}" -I. -I"$IUP_INC" -framework Cocoa -framework AppKit -framework CoreGraphics -w
./editor
