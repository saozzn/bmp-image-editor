#!/usr/bin/env zsh

MAIN_FILE=$(find "/Users/saosun" -name "main.c" 2>/dev/null | head -n 1)
PROJ_DIR=$(dirname "$MAIN_FILE")
cd "$PROJ_DIR"

# 1. Generate lena.bmp and copy it to all iup_test subdirectories
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

with open('lena.bmp', 'wb') as f:
    f.write(header + info + pixels)
"

find "/Users/saosun/All codes/iup_test" -type d 2>/dev/null | while read -r dir; do
  cp "$PROJ_DIR/lena.bmp" "$dir/lena.bmp" 2>/dev/null
done
echo "Synchronized lena.bmp across all project directories."

# 2. Add verbose error logging to image.c to catch header/open mismatches
python3 -c "
with open('image.c', 'r') as f:
    code = f.read()

# Print exact fopen status to terminal
code = code.replace('if (!file)', 'if (!file) { printf(\"[BMP DEBUG] Cannot open file at path\\\\n\");')
code = code.replace('if (!f)', 'if (!f) { printf(\"[BMP DEBUG] Cannot open file at path\\\\n\");')

with open('image.c', 'w') as f:
    f.write(code)
"

# 3. Build and launch
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
GTK_FLAGS=($(pkg-config --cflags --libs gtk+-3.0 2>/dev/null))
IUP_INC=$(dirname "$(find "/Users/saosun" -name "iup.h" 2>/dev/null | head -n 1)")
LIBIUP=$(find "/Users/saosun" -name "libiup.a" 2>/dev/null | head -n 1)

clang -o editor main.c gui.c image.c operations.c "$LIBIUP" "${GTK_FLAGS[@]}" -I. -I"$IUP_INC" -framework Cocoa -framework AppKit -framework CoreGraphics -w
./editor
