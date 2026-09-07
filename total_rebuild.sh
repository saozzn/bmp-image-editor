#!/usr/bin/env zsh

MAIN_FILE=$(find "/Users/saosun" -name "main.c" 2>/dev/null | head -n 1)
PROJ_DIR=$(dirname "$MAIN_FILE")
cd "$PROJ_DIR"

# 1. Clean non-breaking space encoding corruption
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
"

# 2. Write strict 24-bit uncompressed test BMP
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

# 3. Write clean, scoped image.h
cat << 'C_HEADER' > image.h
#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Pixel;

typedef struct {
    int width;
    int height;
    Pixel *data;
} Image;
#pragma pack(pop)

Image *load_bmp(const char *filename);
int save_bmp(const char *filename, const Image *image);
Image *copy_image(const Image *source);
void free_image(Image *image);

#endif
C_HEADER

# 4. Patch open_image_callback in gui.c
python3 -c "
with open('gui.c', 'r') as f:
    code = f.read()

import re
pattern = r'int\s+open_image_callback\s*\([^)]*\)\s*\{[\s\S]*?\n\}'
replacement = '''int open_image_callback(Ihandle *self)
{
    printf(\"[DEBUG] Direct loading lena.bmp...\\\\n\");
    fflush(stdout);

    Image *new_image = load_bmp(\"lena.bmp\");
    if (new_image != NULL)
    {
        if (current_image != NULL) free_image(current_image);
        current_image = new_image;
        printf(\"[SUCCESS] Loaded lena.bmp (%dx%d)\\\\n\", new_image->width, new_image->height);
        fflush(stdout);

        Ihandle *dlg = IupGetDialog(self);
        if (dlg) IupUpdate(dlg);
    }
    else
    {
        printf(\"[ERROR] load_bmp returned NULL\\\\n\");
        fflush(stdout);
    }
    return IUP_DEFAULT;
}'''

code = re.sub(pattern, replacement, code)
with open('gui.c', 'w') as f:
    f.write(code)
"

# 5. Compile and launch
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
GTK_FLAGS=($(pkg-config --cflags --libs gtk+-3.0 2>/dev/null))
IUP_INC=$(dirname "$(find "/Users/saosun" -name "iup.h" 2>/dev/null | head -n 1)")
LIBIUP=$(find "/Users/saosun" -name "libiup.a" 2>/dev/null | head -n 1)

clang -o editor main.c gui.c image.c operations.c "$LIBIUP" "${GTK_FLAGS[@]}" -I. -I"$IUP_INC" -framework Cocoa -framework AppKit -framework CoreGraphics -w
./editor
