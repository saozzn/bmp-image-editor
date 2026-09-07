#!/usr/bin/env zsh

MAIN_FILE=$(find "/Users/saosun" -name "main.c" 2>/dev/null | head -n 1)
PROJ_DIR=$(dirname "$MAIN_FILE")
cd "$PROJ_DIR"

# 1. Inject diagnostic header prints into image.c
python3 -c "
with open('image.c', 'r') as f:
    code = f.read()

if '[DIAGNOSTIC]' not in code:
    target = 'if (file_header.type != 0x4D42'
    replacement = '''
    printf(\"\\n=== BMP HEADER DIAGNOSTIC ===\\n\");
    printf(\"[DIAGNOSTIC] File: %s\\n\", filename);
    printf(\"[DIAGNOSTIC] Header Type: 0x%X (Expected 0x4D42)\\n\", file_header.type);
    printf(\"[DIAGNOSTIC] Bits Per Pixel: %u (Expected 24)\\n\", info_header.bits_per_pixel);
    printf(\"[DIAGNOSTIC] Compression: %u (Expected 0)\\n\", info_header.compression);
    printf(\"[DIAGNOSTIC] Dimensions: %dx%d\\n\", info_header.width, info_header.height);
    printf(\"=============================\\n\\n\");
    fflush(stdout);

    if (file_header.type != 0x4D42'''
    
    code = code.replace(target, replacement)
    with open('image.c', 'w') as f:
        f.write(code)
    print('[SUCCESS] Injected diagnostic logging into image.c')
"

# 2. Compile and launch
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
GTK_FLAGS=($(pkg-config --cflags --libs gtk+-3.0 2>/dev/null))
IUP_INC=$(dirname "$(find "/Users/saosun" -name "iup.h" 2>/dev/null | head -n 1)")
LIBIUP=$(find "/Users/saosun" -name "libiup.a" 2>/dev/null | head -n 1)

echo "=== Compiling Application ==="
clang -o editor main.c gui.c image.c operations.c "$LIBIUP" "${GTK_FLAGS[@]}" -I. -I"$IUP_INC" -framework Cocoa -framework AppKit -framework CoreGraphics -w

if [ -f ./editor ]; then
    echo "=== Launching Editor ==="
    ./editor
else
    echo "[FAIL] Compilation failed!"
fi
