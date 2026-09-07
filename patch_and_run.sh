#!/usr/bin/env zsh

MAIN_FILE=$(find "/Users/saosun" -name "main.c" 2>/dev/null | head -n 1)
PROJ_DIR=$(dirname "$MAIN_FILE")
cd "$PROJ_DIR"

# Write standalone Python patcher using quoted EOF to prevent Zsh globbing errors
cat << 'PYEOF' > patch_gui.py
import os

with open('gui.c', 'r') as f:
    code = f.read()

# Remove all existing open_image_callback functions
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

# Append dynamic file selection callback using IUP file dialog
dynamic_callback = r'''
int open_image_callback(Ihandle *self)
{
    Ihandle *file_dialog = IupFileDlg();
    IupSetAttribute(file_dialog, "DIALOGTYPE", "OPEN");
    IupSetAttribute(file_dialog, "TITLE", "Select BMP Image");
    IupSetAttribute(file_dialog, "EXTFILTER", "BMP Files (*.bmp)|*.bmp|All Files (*.*)|*.*|");

    IupPopup(file_dialog, IUP_CENTER, IUP_CENTER);

    if (IupGetInt(file_dialog, "STATUS") != -1)
    {
        char *filename = IupGetAttribute(file_dialog, "VALUE");
        if (filename && filename[0] != '\0')
        {
            printf("[GUI] Selected file: %s\n", filename);
            fflush(stdout);

            Image *new_image = load_bmp(filename);
            if (new_image != NULL)
            {
                if (current_image != NULL)
                {
                    free_image(current_image);
                }
                current_image = new_image;
                printf("[SUCCESS] Loaded %s (%dx%d)\n", filename, new_image->width, new_image->height);
                fflush(stdout);

                Ihandle *dlg = IupGetDialog(self);
                if (dlg) IupUpdate(dlg);
                Ihandle *canvas = IupGetHandle("canvas");
                if (canvas) IupUpdate(canvas);
            }
            else
            {
                printf("[ERROR] Failed to load BMP file: %s\n", filename);
                fflush(stdout);
            }
        }
    }
    else
    {
        printf("[GUI] File selection cancelled.\n");
        fflush(stdout);
    }

    IupDestroy(file_dialog);
    return IUP_DEFAULT;
}
'''

code += '\n' + dynamic_callback

with open('gui.c', 'w') as f:
    f.write(code)

print('[SUCCESS] gui.c updated cleanly with dynamic file dialog.')
PYEOF

python3 patch_gui.py
rm -f patch_gui.py

# Compile and launch
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
