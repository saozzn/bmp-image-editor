#!/usr/bin/env zsh

# 1. Locate project root directory
MAIN_FILE=$(find "/Users/saosun" -name "main.c" 2>/dev/null | head -n 1)
if [ -z "$MAIN_FILE" ]; then
  echo "Error: main.c not found"
  exit 1
fi
PROJ_DIR=$(dirname "$MAIN_FILE")
cd "$PROJ_DIR"

# Delete old broken backups to start totally fresh
rm -f gui.c.bak image.c.bak image.h.bak

# 2. Sanitize source files and clean non-breaking spaces (\xa0)
python3 -c "
import os
for root, dirs, files in os.walk('.'):
    for f in files:
        if f.endswith('.c') or f.endswith('.h'):
            p = os.path.join(root, f)
            with open(p, 'rb') as fp:
                content = fp.read()
            content = content.replace(b'\xc2\xa0', b' ').replace(b'\xa0', b' ')
            with open(p, 'wb') as fp:
                fp.write(content)
"

# 3. Create a guaranteed 24-bit uncompressed RGB lena.bmp in current directory
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
print('[FILE] Generated 24-bit test image at lena.bmp')
"

# 4. Overwrite open_image_callback in gui.c completely without conditional dialog wrappers
python3 -c "
import re

with open('gui.c', 'r') as f:
    code = f.read()

pattern = r'int\s+open_image_callback\s*\([^)]*\)\s*\{'
match = re.search(pattern, code)

if match:
    start = match.start()
    brace_count = 0
    end = -1
    for i in range(match.end() - 1, len(code)):
        if code[i] == '{':
            brace_count += 1
        elif code[i] == '}':
            brace_count -= 1
            if brace_count == 0:
                end = i + 1
                break

    if end != -1:
        new_func = '''int open_image_callback(Ihandle *self)
{
    printf(\"[DEBUG] Open button clicked! Direct loading lena.bmp...\\\\n\");
    fflush(stdout);

    Image *new_image = load_bmp(\"lena.bmp\");
    if (new_image != NULL)
    {
        if (current_image != NULL)
        {
            free_image(current_image);
        }
        current_image = new_image;
        printf(\"[SUCCESS] Loaded lena.bmp (%dx%d)\\\\n\", new_image->width, new_image->height);
        fflush(stdout);

        Ihandle *dlg = IupGetDialog(self);
        if (dlg) IupUpdate(dlg);
        Ihandle *canvas = IupGetHandle(\"canvas\");
        if (canvas) IupUpdate(canvas);
    }
    else
    {
        printf(\"[ERROR] load_bmp failed to open lena.bmp\\\\n\");
        fflush(stdout);
    }
    return IUP_DEFAULT;
}'''
        code = code[:start] + new_func + code[end:]

        with open('gui.c', 'w') as f:
            f.write(code)
        print('[PATCH] Updated open_image_callback in gui.c')
else:
    print('[WARN] Could not locate open_image_callback pattern in gui.c')
"

# 5. Print open_image_callback to Terminal for verification
echo "\n=== VERIFYING PATCHED OPEN CALLBACK IN GUI.C ==="
grep -A 25 "open_image_callback" gui.c

# 6. Compile and launch application
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
  echo "Compilation failed!"
fi
