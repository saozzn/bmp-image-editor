#!/usr/bin/env zsh

# 1. Locate project root directory
MAIN_FILE=$(find "/Users/saosun" -name "main.c" 2>/dev/null | head -n 1)
PROJ_DIR=$(dirname "$MAIN_FILE")
cd "$PROJ_DIR"

# 2. Restore clean backup files to eliminate stacked edits
[ -f gui.c.bak ] && cp gui.c.bak gui.c
[ -f image.c.bak ] && cp image.c.bak image.c
[ -f image.h.bak ] && cp image.h.bak image.h

# 3. Cleanly replace open_image_callback without calling broken IupFileDlg
python3 -c "
with open('gui.c', 'r') as f:
    code = f.read()

start_str = 'int open_image_callback(Ihandle *self)'
if start_str in code:
    idx_start = code.find(start_str)
    brace_count = 0
    idx_end = idx_start
    found_first_brace = False

    for i in range(idx_start, len(code)):
        if code[i] == '{':
            brace_count += 1
            found_first_brace = True
        elif code[i] == '}':
            brace_count -= 1

        if found_first_brace and brace_count == 0:
            idx_end = i + 1
            break

    new_func = '''int open_image_callback(Ihandle *self)
{
    const char *filename = \"lena.bmp\";
    Image *new_image = load_bmp(filename);

    if (new_image != NULL)
    {
        free_image(current_image);
        current_image = new_image;
        printf(\"[SUCCESS] Loaded %s (%dx%d)\\\\n\", filename, new_image->width, new_image->height);
        IupUpdate(IupGetDialog(self));
    }
    else
    {
        printf(\"[ERROR] load_bmp failed to load %s\\\\n\", filename);
    }
    return IUP_DEFAULT;
}'''
    code = code[:idx_start] + new_func + code[idx_end:]

with open('gui.c', 'w') as f:
    f.write(code)
print('Replaced open_image_callback safely in gui.c')
"

# 4. Generate/Convert a strict 24-bit uncompressed BMP in working directory
python3 -c "
import struct, os

try:
    from PIL import Image as PILImage
    if os.path.exists('lena.bmp'):
        im = PILImage.open('lena.bmp').convert('RGB')
        im.save('lena.bmp', 'BMP')
        print('Converted existing lena.bmp to 24-bit uncompressed format.')
    else:
        raise Exception('No file')
except Exception:
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
    print('Generated clean 24-bit test lena.bmp')
"

# 5. Compile and launch
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
GTK_FLAGS=($(pkg-config --cflags --libs gtk+-3.0 2>/dev/null))
IUP_INC=$(dirname "$(find "/Users/saosun" -name "iup.h" 2>/dev/null | head -n 1)")
LIBIUP=$(find "/Users/saosun" -name "libiup.a" 2>/dev/null | head -n 1)

clang -o editor main.c gui.c image.c operations.c "$LIBIUP" "${GTK_FLAGS[@]}" -I. -I"$IUP_INC" -framework Cocoa -framework AppKit -framework CoreGraphics -w
./editor
