#!/usr/bin/env zsh

MAIN_FILE=$(find "/Users/saosun" -name "main.c" 2>/dev/null | head -n 1)
PROJ_DIR=$(dirname "$MAIN_FILE")
cd "$PROJ_DIR"

# 1. Clean invisible non-breaking space characters from source files
python3 -c "
import os
for root, dirs, files in os.walk('.'):
    for f in files:
        if f.endswith('.c') or f.endswith('.h'):
            p = os.path.join(root, f)
            with open(p, 'rb') as fp:
                c = fp.read().replace(b'\xc2\xa0', b' ').replace(b'\xa0', b' ')
            with open(p, 'wb') as fp:
                fp.write(c)
print('[1/3] Cleaned source file encodings')
"

# 2. Generate valid 24-bit uncompressed RGB lena.bmp
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
print('[2/3] Generated valid 24-bit lena.bmp')
"

# 3. Safely update open_image_callback in gui.c
python3 -c "
with open('gui.c', 'r') as f:
    content = f.read()

import re
pattern = r'int\s+open_image_callback\s*\([^)]*\)\s*\{[\s\S]*?\n\}'
new_callback = '''int open_image_callback(Ihandle *self)
{
    printf(\"[BMP] Open button clicked! Loading lena.bmp...\\\\n\");
    fflush(stdout);

    Image *new_image = load_bmp(\"lena.bmp\");
    if (new_image != NULL)
    {
        if (current_image != NULL) free_image(current_image);
        current_image = new_image;
        printf(\"[BMP SUCCESS] Loaded lena.bmp (%dx%d)\\\\n\", new_image->width, new_image->height);
        fflush(stdout);
        IupUpdate(IupGetDialog(self));
    }
    else
    {
        printf(\"[BMP ERROR] load_bmp returned NULL\\\\n\");
        fflush(stdout);
    }
    return IUP_DEFAULT;
}'''

content = re.sub(pattern, new_callback, content)
with open('gui.c', 'w') as f:
    f.write(content)
print('[3/3] Updated open_image_callback in gui.c')
"

# 4. Compile and launch
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
GTK_FLAGS=($(pkg-config --cflags --libs gtk+-3.0 2>/dev/null))
IUP_INC=$(dirname "$(find "/Users/saosun" -name "iup.h" 2>/dev/null | head -n 1)")
LIBIUP=$(find "/Users/saosun" -name "libiup.a" 2>/dev/null | head -n 1)

clang -o editor main.c gui.c image.c operations.c "$LIBIUP" "${GTK_FLAGS[@]}" -I. -I"$IUP_INC" -framework Cocoa -framework AppKit -framework CoreGraphics -w

if [ -f ./editor ]; then
    ./editor
else
    echo "Compilation failed!"
fi
