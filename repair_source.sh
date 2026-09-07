#!/usr/bin/env zsh

MAIN_FILE=$(find "/Users/saosun" -name "main.c" 2>/dev/null | head -n 1)
PROJ_DIR=$(dirname "$MAIN_FILE")
cd "$PROJ_DIR"

cat << 'PYEOF' > repair.py
import re

# 1. Fix image.c multi-line string literal error
with open('image.c', 'r') as f:
    img_code = f.read()

# Strip any broken multi-line printf blocks
img_code = re.sub(r'printf\("\s*\n[\s\S]*?=============================\\n\\n"\);', '', img_code)

# Insert clean single-line diagnostic logging
if '[DIAGNOSTIC]' not in img_code:
    target = 'if (file_header.type != 0x4D42'
    diag = 'printf("[DIAGNOSTIC] File: %s | BPP: %u | Compression: %u\\n", filename, info_header.bits_per_pixel, info_header.compression);\n    fflush(stdout);\n    if (file_header.type != 0x4D42'
    img_code = img_code.replace(target, diag)

with open('image.c', 'w') as f:
    f.write(img_code)
print('[1/2] Fixed image.c string syntax.')

# 2. Fix gui.c start_gui function header signature
with open('gui.c', 'r') as f:
    gui_code = f.read()

# Restore missing parameters and opening brace before image_display
gui_code = re.sub(r'void\s+start_gui\s*\([\s\S]*?(?=image_display\s*=)', 'void start_gui(int argc, char **argv)\n{\n    ', gui_code)

with open('gui.c', 'w') as f:
    f.write(gui_code)
print('[2/2] Fixed gui.c start_gui definition.')
PYEOF

python3 repair.py
rm -f repair.py

# 3. Compile and Execute
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
GTK_FLAGS=($(pkg-config --cflags --libs gtk+-3.0 2>/dev/null))
IUP_INC=$(dirname "$(find "/Users/saosun" -name "iup.h" 2>/dev/null | head -n 1)")
LIBIUP=$(find "/Users/saosun" -name "libiup.a" 2>/dev/null | head -n 1)

echo "\n=== Compiling Application ==="
clang -o editor main.c gui.c image.c operations.c "$LIBIUP" "${GTK_FLAGS[@]}" -I. -I"$IUP_INC" -framework Cocoa -framework AppKit -framework CoreGraphics -w

if [ -f ./editor ]; then
    echo "=== Launching Editor ==="
    ./editor
else
    echo "[FAIL] Compilation failed!"
fi
