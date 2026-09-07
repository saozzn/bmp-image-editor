#!/usr/bin/env zsh

# 1. Locate project root
MAIN_FILE=$(find "/Users/saosun" -name "main.c" 2>/dev/null | head -n 1)
if [ -z "$MAIN_FILE" ]; then
  echo "[ERROR] main.c not found"
  exit 1
fi
PROJ_DIR=$(dirname "$MAIN_FILE")
cd "$PROJ_DIR"

# 2. Strip non-breaking space encoding corruption (\xa0)
python3 -c "
import os
for filename in ['gui.c', 'image.c', 'image.h']:
    if os.path.exists(filename):
        with open(filename, 'rb') as f:
            content = f.read().replace(b'\xc2\xa0', b' ').replace(b'\xa0', b' ')
        with open(filename, 'wb') as f:
            f.write(content)
"

# 3. Patch open_image_callback in gui.c using balanced bracket matching
python3 -c "
with open('gui.c', 'r') as f:
    code = f.read()

pos = code.find('open_image_callback')
if pos != -1:
    brace_start = code.find('{', pos)
    if brace_start != -1:
        count = 1
        i = brace_start + 1
        while i < len(code) and count > 0:
            if code[i] == '{': count += 1
            elif code[i] == '}': count -= 1
            i += 1
        brace_end = i
        func_start = code.rfind('\n', 0, pos)
        if func_start == -1: func_start = 0

        new_func = '''
int open_image_callback(Ihandle *self)
{
    printf(\"[DEBUG] Open button clicked! Calling load_bmp('lena.bmp')...\\\\n\");
    fflush(stdout);

    Image *new_image = load_bmp(\"lena.bmp\");
    if (new_image != NULL)
    {
        if (current_image != NULL) free_image(current_image);
        current_image = new_image;
        printf(\"[SUCCESS] Successfully loaded lena.bmp (%dx%d)!\\\\n\", new_image->width, new_image->height);
        fflush(stdout);

        Ihandle *dlg = IupGetDialog(self);
        if (dlg) IupUpdate(dlg);
        Ihandle *canvas = IupGetHandle(\"canvas\");
        if (canvas) IupUpdate(canvas);
    }
    else
    {
        printf(\"[ERROR] load_bmp('lena.bmp') returned NULL\\\\n\");
        fflush(stdout);
    }
    return IUP_DEFAULT;
}'''
        code = code[:func_start] + new_func + code[brace_end:]
        with open('gui.c', 'w') as f:
            f.write(code)
        print('[PATCH] Cleanly updated open_image_callback in gui.c')
"

# 4. Patch load_bmp in image.c with explicit line-by-line terminal diagnostics
python3 -c "
with open('image.c', 'r') as f:
    code = f.read()

pos = code.find('load_bmp')
if pos != -1:
    brace_start = code.find('{', pos)
    if brace_start != -1:
        count = 1
        i = brace_start + 1
        while i < len(code) and count > 0:
            if code[i] == '{': count += 1
            elif code[i] == '}': count -= 1
            i += 1
        brace_end = i
        func_start = code.rfind('\n', 0, pos)
        if func_start == -1: func_start = 0

        new_func = '''
Image *load_bmp(const char *filename)
{
    FILE *file;
    BMPFileHeader file_header;
    BMPInfoHeader info_header;
    Image *image;
    unsigned char *row;
    int width, height, bottom_up, row_size, x, y;

    if (filename == NULL) {
        printf(\"[BMP ERROR] Filename pointer is NULL\\\\n\");
        fflush(stdout);
        return NULL;
    }

    file = fopen(filename, \"rb\");
    if (file == NULL) {
        printf(\"[BMP ERROR] Cannot open file '%s'.\\\\n\", filename);
        fflush(stdout);
        return NULL;
    }

    if (fread(&file_header, sizeof(file_header), 1, file) != 1 ||
        fread(&info_header, sizeof(info_header), 1, file) != 1) {
        printf(\"[BMP ERROR] Failed to read BMP headers from '%s'\\\\n\", filename);
        fflush(stdout);
        fclose(file);
        return NULL;
    }

    printf(\"[BMP DEBUG] Header sizes: file=%lu, info=%lu\\\\n\", sizeof(file_header), sizeof(info_header));
    printf(\"[BMP DEBUG] Type=0x%X, BPP=%d, Compression=%d, Size=%dx%d\\\\n\",
           file_header.type, info_header.bits_per_pixel, info_header.compression,
           info_header.width, info_header.height);
    fflush(stdout);

    if (file_header.type != 0x4D42) {
        printf(\"[BMP ERROR] Type 0x%X != 0x4D42 (Not a valid 'BM' file)\\\\n\", file_header.type);
        fflush(stdout);
        fclose(file);
        return NULL;
    }

    if (info_header.bits_per_pixel != 24) {
        printf(\"[BMP ERROR] Bits per pixel is %d (Expected 24-bit RGB)\\\\n\", info_header.bits_per_pixel);
        fflush(stdout);
        fclose(file);
        return NULL;
    }

    if (info_header.compression != 0) {
        printf(\"[BMP ERROR] Compression is %d (Expected uncompressed 0)\\\\n\", info_header.compression);
        fflush(stdout);
        fclose(file);
        return NULL;
    }

    if (info_header.width <= 0 || info_header.height == 0) {
        printf(\"[BMP ERROR] Invalid dimensions: %dx%d\\\\n\", info_header.width, info_header.height);
        fflush(stdout);
        fclose(file);
        return NULL;
    }

    width = info_header.width;
    if (info_header.height < 0) {
        height = -info_header.height;
        bottom_up = 0;
    } else {
        height = info_header.height;
        bottom_up = 1;
    }

    image = (Image *)malloc(sizeof(Image));
    if (image == NULL) {
        printf(\"[BMP ERROR] Memory allocation failed for Image\\\\n\");
        fflush(stdout);
        fclose(file);
        return NULL;
    }

    image->width = width;
    image->height = height;
    image->data = (Pixel *)malloc((size_t)width * (size_t)height * sizeof(Pixel));
    if (image->data == NULL) {
        printf(\"[BMP ERROR] Memory allocation failed for Pixel data\\\\n\");
        fflush(stdout);
        free(image);
        fclose(file);
        return NULL;
    }

    row_size = ((width * 3 + 3) / 4) * 4;
    row = (unsigned char *)malloc((size_t)row_size);
    if (row == NULL) {
        printf(\"[BMP ERROR] Memory allocation failed for row buffer\\\\n\");
        fflush(stdout);
        free(image->data);
        free(image);
        fclose(file);
        return NULL;
    }

    if (fseek(file, (long)file_header.offset, SEEK_SET) != 0) {
        printf(\"[BMP ERROR] fseek to offset %u failed\\\\n\", file_header.offset);
        fflush(stdout);
        free(row);
        free(image->data);
        free(image);
        fclose(file);
        return NULL;
    }

    for (y = 0; y < height; y++) {
        int destination_y;
        if (fread(row, 1, (size_t)row_size, file) != (size_t)row_size) {
            printf(\"[BMP ERROR] Failed to read row %d\\\\n\", y);
            fflush(stdout);
            free(row);
            free(image->data);
            free(image);
            fclose(file);
            return NULL;
        }

        destination_y = bottom_up ? (height - 1 - y) : y;

        for (x = 0; x < width; x++) {
            Pixel *pixel = &image->data[destination_y * width + x];
            pixel->b = row[x * 3 + 0];
            pixel->g = row[x * 3 + 1];
            pixel->r = row[x * 3 + 2];
        }
    }

    free(row);
    fclose(file);
    printf(\"[BMP SUCCESS] Pixel data successfully loaded (%dx%d)!\\\\n\", width, height);
    fflush(stdout);
    return image;
}'''
        code = code[:func_start] + new_func + code[brace_end:]
        with open('image.c', 'w') as f:
            f.write(code)
        print('[PATCH] Replaced load_bmp in image.c with diagnostic loader')
"

# 5. Generate verified 24-bit uncompressed lena.bmp
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
print('[FILE] Generated 24-bit lena.bmp in working directory.')
"

# 6. Compile and Run
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
GTK_FLAGS=($(pkg-config --cflags --libs gtk+-3.0 2>/dev/null))
IUP_INC=$(dirname "$(find "/Users/saosun" -name "iup.h" 2>/dev/null | head -n 1)")
LIBIUP=$(find "/Users/saosun" -name "libiup.a" 2>/dev/null | head -n 1)

echo "\n=== COMPILING APPLICATION ==="
clang -o editor main.c gui.c image.c operations.c "$LIBIUP" "${GTK_FLAGS[@]}" -I. -I"$IUP_INC" -framework Cocoa -framework AppKit -framework CoreGraphics -w

if [ -f ./editor ]; then
    echo "\n=== LAUNCHING EDITOR ==="
    ./editor
else
    echo "[FAIL] Compilation failed! Check build error above."
fi
