#!/usr/bin/env zsh

MAIN_FILE=$(find "/Users/saosun" -name "main.c" 2>/dev/null | head -n 1)
PROJ_DIR=$(dirname "$MAIN_FILE")
cd "$PROJ_DIR"

# 1. Restore clean original image.c
[ -f image.c.bak ] && cp image.c.bak image.c

# 2. Print header validation logic from image.c for inspection
python3 -c "
with open('image.c', 'r') as f:
    code = f.read()

print('=== Image Loader Logic in image.c ===')
for idx, line in enumerate(code.splitlines(), 1):
    if any(k in line for k in ['fopen', 'fread', 'bfType', 'biBitCount', 'biCompression', 'return NULL', 'return 0']):
        print(f'Line {idx}: {line.strip()}')
"

# 3. Add safe debug log immediately after opening the file
python3 -c "
with open('image.c', 'r') as f:
    code = f.read()

code = code.replace('file = fopen(', 'printf(\"[BMP DEBUG] Attempting to open: %s\\\\n\", filename); file = fopen(')
code = code.replace('f = fopen(', 'printf(\"[BMP DEBUG] Attempting to open: %s\\\\n\", filename); f = fopen(')

with open('image.c', 'w') as f:
    f.write(code)
"

# 4. Compile and launch
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
GTK_FLAGS=($(pkg-config --cflags --libs gtk+-3.0 2>/dev/null))
IUP_INC=$(dirname "$(find "/Users/saosun" -name "iup.h" 2>/dev/null | head -n 1)")
LIBIUP=$(find "/Users/saosun" -name "libiup.a" 2>/dev/null | head -n 1)

clang -o editor main.c gui.c image.c operations.c "$LIBIUP" "${GTK_FLAGS[@]}" -I. -I"$IUP_INC" -framework Cocoa -framework AppKit -framework CoreGraphics -w
./editor
