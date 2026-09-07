#!/usr/bin/env zsh

MAIN_FILE=$(find "/Users/saosun" -name "main.c" 2>/dev/null | head -n 1)
PROJ_DIR=$(dirname "$MAIN_FILE")
cd "$PROJ_DIR"

# 1. Restore clean original source files
[ -f gui.c.bak ] && cp gui.c.bak gui.c
[ -f image.c.bak ] && cp image.c.bak image.c
[ -f image.h.bak ] && cp image.h.bak image.h

# 2. Patch gui.c to fix macOS file dialog & add canvas redraw
python3 -c "
with open('gui.c', 'r') as f:
    code = f.read()

# Locate canvas variable name
import re
canvas_var = 'canvas'
match = re.search(r'Ihandle\s*\*\s*(\w+)\s*=.*IupCanvas', code)
if match:
    canvas_var = match.group(1)

# Remove GTK macOS filter bug triggers
code = re.sub(r'IupSetAttribute\s*\(\s*file_dialog\s*,\s*\"FILTER\".*?\);', '// filter removed', code)
code = re.sub(r'IupSetAttribute\s*\(\s*file_dialog\s*,\s*\"FILTERINFO\".*?\);', '// filterinfo removed', code)

# Log file dialog path selection
code = code.replace(
    'filename = IupGetAttribute(file_dialog, \"VALUE\");',
    'filename = IupGetAttribute(file_dialog, \"VALUE\");\n        printf(\"[DIALOG] Selected file: %s\\\\n\", filename ? filename : \"NULL\");'
)

# Force UI redraw upon successful load
code = code.replace(
    'current_image = new_image;',
    'current_image = new_image;\n            printf(\"[SUCCESS] Loaded image: %dx%d\\\\n\", new_image->width, new_image->height);\n            IupUpdate(' + canvas_var + ');'
)

with open('gui.c', 'w') as f:
    f.write(code)
print('Patched gui.c file dialog and canvas redraw logic.')
"

# 3. Compile and launch application
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
GTK_FLAGS=($(pkg-config --cflags --libs gtk+-3.0 2>/dev/null))
IUP_INC=$(dirname "$(find "/Users/saosun" -name "iup.h" 2>/dev/null | head -n 1)")
LIBIUP=$(find "/Users/saosun" -name "libiup.a" 2>/dev/null | head -n 1)

clang -o editor main.c gui.c image.c operations.c "$LIBIUP" "${GTK_FLAGS[@]}" -I. -I"$IUP_INC" -framework Cocoa -framework AppKit -framework CoreGraphics -w
./editor
