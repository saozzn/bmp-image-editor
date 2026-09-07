#!/usr/bin/env zsh

# 1. Locate directories and files
MAIN_FILE=$(find "/Users/saosun" -name "main.c" 2>/dev/null | head -n 1)
if [ -z "$MAIN_FILE" ]; then
  echo "Error: main.c not found"
  exit 1
fi
PROJ_DIR=$(dirname "$MAIN_FILE")
cd "$PROJ_DIR"

IUP_INC=$(dirname "$(find "/Users/saosun" -name "iup.h" 2>/dev/null | head -n 1)")
if [ -z "$IUP_INC" ]; then
  echo "Error: iup.h not found"
  exit 1
fi

LIBIUP="$PROJ_DIR/libiup.a"
if [ ! -f "$LIBIUP" ]; then
  LIBIUP=$(find "/Users/saosun" -name "libiup.a" 2>/dev/null | head -n 1)
fi

echo "Project directory: $PROJ_DIR"
echo "IUP include path:  $IUP_INC"
echo "Static library:    $LIBIUP"

# Restore clean gui.c if backup exists
[ -f gui.c.bak ] && cp gui.c.bak gui.c || cp gui.c gui.c.bak

# 2. Generate a valid 24-bit uncompressed RGB BMP named lena.bmp
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
print('Generated 24-bit lena.bmp')
"

# 3. Add struct packing alignment to handle 64-bit arm64 memory offsets
python3 -c "
import os
for fname in ['image.h', 'image.c']:
    if os.path.exists(fname):
        with open(fname, 'r') as f:
            c = f.read()
        if '#pragma pack' not in c:
            c = '#pragma pack(push, 1)\n' + c + '\n#pragma pack(pop)\n'
            with open(fname, 'w') as f:
                f.write(c)
"

# 4. Patch gui.c to assign lena.bmp directly without triggering file dialog crashes
python3 -c "
with open('gui.c', 'r') as f:
    code = f.read()

lines = code.splitlines()
new_lines = []
for line in lines:
    s = line.strip()
    if 'IupFileDlg' in s or 'IupPopup' in s or ('IupDestroy' in s and 'filedlg' in s):
        new_lines.append('// ' + line)
    elif 'IupSetAttribute' in s and 'filedlg' in s:
        new_lines.append('// ' + line)
    elif 'IupGetAttribute' in s and 'filedlg' in s:
        indent = line[:len(line) - len(line.lstrip())]
        new_lines.append(indent + 'filename = \"lena.bmp\";')
    else:
        new_lines.append(line)

with open('gui.c', 'w') as f:
    f.write('\n'.join(new_lines))
print('Patched gui.c successfully')
"

# 5. Build and launch application
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
GTK_FLAGS=($(pkg-config --cflags --libs gtk+-3.0 2>/dev/null))

clang -o editor main.c gui.c image.c operations.c "$LIBIUP" "${GTK_FLAGS[@]}" -I. -I"$IUP_INC" -framework Cocoa -framework AppKit -framework CoreGraphics -w

if [ -f ./editor ]; then
  echo "Launching application..."
  ./editor
else
  echo "Build failed. Check compiler output above."
fi
