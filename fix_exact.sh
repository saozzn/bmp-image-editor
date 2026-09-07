#!/usr/bin/env zsh

MAIN_FILE=$(find "/Users/saosun" -name "main.c" 2>/dev/null | head -n 1)
PROJ_DIR=$(dirname "$MAIN_FILE")
cd "$PROJ_DIR"

# 1. Restore clean backups
[ -f gui.c.bak ] && cp gui.c.bak gui.c
[ -f image.c.bak ] && cp image.c.bak image.c
[ -f image.h.bak ] && cp image.h.bak image.h

ABS_LENA="$PROJ_DIR/lena.bmp"

# 2. Generate a valid 24-bit uncompressed RGB BMP
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
"

# 3. Force __attribute__((packed)) on structs in image.h for 64-bit ARM alignment
python3 -c "
with open('image.h', 'r') as f:
    content = f.read()

lines = content.splitlines()
new_lines = []
for line in lines:
    if line.strip().startswith('}') and ';' in line and '__attribute__' not in line:
        new_lines.append(line.replace('}', '} __attribute__((packed))'))
    else:
        new_lines.append(line)

with open('image.h', 'w') as f:
    f.write('\n'.join(new_lines))
"

# 4. Inject safe diagnostic logging inside image.c
python3 -c "
with open('image.c', 'r') as f:
    code = f.read()

code = code.replace('fopen(filename', 'printf(\"[LOADER] Path passed: %s\\\\n\", filename); fopen(filename')
code = code.replace('return NULL;', '{ printf(\"[LOADER FAIL] Rejected at line %d in image.c\\\\n\", __LINE__); return NULL; }')

with open('image.c', 'w') as f:
    f.write(code)
"

# 5. Patch gui.c to assign absolute path to filename
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
        new_lines.append(indent + 'filename = \"' + '$ABS_LENA' + '\";')
    else:
        new_lines.append(line)

with open('gui.c', 'w') as f:
    f.write('\n'.join(new_lines))
"

# 6. Compile and launch
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
GTK_FLAGS=($(pkg-config --cflags --libs gtk+-3.0 2>/dev/null))
IUP_INC=$(dirname "$(find "/Users/saosun" -name "iup.h" 2>/dev/null | head -n 1)")
LIBIUP=$(find "/Users/saosun" -name "libiup.a" 2>/dev/null | head -n 1)

clang -o editor main.c gui.c image.c operations.c "$LIBIUP" "${GTK_FLAGS[@]}" -I. -I"$IUP_INC" -framework Cocoa -framework AppKit -framework CoreGraphics -w
./editor
