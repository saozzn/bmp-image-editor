#!/usr/bin/env zsh

MAIN_FILE=$(find "/Users/saosun" -name "main.c" 2>/dev/null | head -n 1)
PROJ_DIR=$(dirname "$MAIN_FILE")
cd "$PROJ_DIR"

# 1. Restore clean original files if backed up
[ -f image.c.bak ] && cp image.c.bak image.c || cp image.c image.c.bak

# 2. Add detailed stdout debugging to every return/failure point in image.c
python3 -c "
with open('image.c', 'r') as f:
    lines = f.readlines()

new_lines = []
for line in lines:
    if 'return NULL;' in line or 'return 0;' in line:
        indent = line[:len(line) - len(line.lstrip())]
        new_lines.append(indent + 'printf(\"[BMP ERROR] Failed at line %d in image.c\\n\", __LINE__);\n')
    new_lines.append(line)

with open('image.c', 'w') as f:
    f.writelines(new_lines)
print('Added line-number debugging to image.c')
"

# 3. Print image.c around file opening / reading for inspection
echo "=== image.c Load Function Header ==="
grep -n -C 5 "fopen" image.c

# 4. Compile and launch
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
GTK_FLAGS=($(pkg-config --cflags --libs gtk+-3.0 2>/dev/null))
IUP_INC=$(dirname "$(find "/Users/saosun" -name "iup.h" 2>/dev/null | head -n 1)")
LIBIUP=$(find "/Users/saosun" -name "libiup.a" 2>/dev/null | head -n 1)

clang -o editor main.c gui.c image.c operations.c "$LIBIUP" "${GTK_FLAGS[@]}" -I. -I"$IUP_INC" -framework Cocoa -framework AppKit -framework CoreGraphics -w
./editor
