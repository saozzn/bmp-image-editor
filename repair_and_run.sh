#!/usr/bin/env zsh

MAIN_FILE=$(find "/Users/saosun" -name "main.c" 2>/dev/null | head -n 1)
PROJ_DIR=$(dirname "$MAIN_FILE")
cd "$PROJ_DIR"

# 1. Purge duplicate functions and fix broken quote syntax in gui.c
python3 -c "
with open('gui.c', 'r') as f:
    code = f.read()

# Fix backslash-escaped quotes and literal escaped newlines
code = code.replace(r'\"', '\"').replace(r'\\\\n', r'\\n')

# Strip ALL existing open_image_callback instances to fix redefinition
while 'open_image_callback' in code:
    idx = code.find('open_image_callback')
    func_start = code.rfind('int', 0, idx)
    if func_start == -1: func_start = idx

    brace_start = code.find('{', idx)
    if brace_start == -1:
        semicolon = code.find(';', idx)
        code = code[:func_start] + code[semicolon+1:]
        continue

    depth = 1
    i = brace_start + 1
    while i < len(code) and depth > 0:
        if code[i] == '{': depth += 1
        elif code[i] == '}': depth -= 1
        i += 1
    code = code[:func_start] + code[i:]

# Append single clean callback function
clean_callback = '''
int open_image_callback(Ihandle *self)
{
    printf(\"[BMP] Open button clicked! Direct loading lena.bmp...\\n\");
    fflush(stdout);

    Image *new_image = load_bmp(\"lena.bmp\");
    if (new_image != NULL)
    {
        if (current_image != NULL) free_image(current_image);
        current_image = new_image;
        printf(\"[BMP SUCCESS] Loaded lena.bmp (%dx%d)\\n\", new_image->width, new_image->height);
        fflush(stdout);
        Ihandle *dlg = IupGetDialog(self);
        if (dlg) IupUpdate(dlg);
    }
    else
    {
        printf(\"[BMP ERROR] load_bmp returned NULL\\n\");
        fflush(stdout);
    }
    return IUP_DEFAULT;
}
'''

code += '\n' + clean_callback

with open('gui.c', 'w') as f:
    f.write(code)
print('[1/3] Successfully cleaned gui.c syntax and duplicate callbacks.')
"

# 2. Generate valid uncompressed 24-bit test image
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
print('[2/3] Generated valid 24-bit test lena.bmp.')
"

# 3. Compile and Execute
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
GTK_FLAGS=($(pkg-config --cflags --libs gtk+-3.0 2>/dev/null))
IUP_INC=$(dirname "$(find "/Users/saosun" -name "iup.h" 2>/dev/null | head -n 1)")
LIBIUP=$(find "/Users/saosun" -name "libiup.a" 2>/dev/null | head -n 1)

echo "[3/3] Compiling..."
clang -o editor main.c gui.c image.c operations.c "$LIBIUP" "${GTK_FLAGS[@]}" -I. -I"$IUP_INC" -framework Cocoa -framework AppKit -framework CoreGraphics -w

if [ -f ./editor ]; then
    echo "=== Launching Editor ==="
    ./editor
else
    echo "Build failed!"
fi
