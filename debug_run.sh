#!/usr/bin/env zsh

MAIN_FILE=$(find "/Users/saosun" -name "main.c" 2>/dev/null | head -n 1)
if [ -z "$MAIN_FILE" ]; then
  echo "Error: main.c not found"
  exit 1
fi
PROJ_DIR=$(dirname "$MAIN_FILE")
cd "$PROJ_DIR"

echo "=== MAIN.C CONTENT ==="
cat main.c

echo "\n=== COMPILING (VERBOSE) ==="
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH"
GTK_FLAGS=($(pkg-config --cflags --libs gtk+-3.0 2>/dev/null))
IUP_INC=$(dirname "$(find "/Users/saosun" -name "iup.h" 2>/dev/null | head -n 1)")
LIBIUP=$(find "/Users/saosun" -name "libiup.a" 2>/dev/null | head -n 1)

clang -o editor main.c gui.c image.c operations.c "$LIBIUP" "${GTK_FLAGS[@]}" -I. -I"$IUP_INC" -framework Cocoa -framework AppKit -framework CoreGraphics 2>&1 | tee build.log

if [ -f ./editor ]; then
  echo "\n=== RUNNING ./editor ==="
  ./editor 2>&1 | tee run.log
else
  echo "\n[FAIL] Compilation produced no executable."
fi
