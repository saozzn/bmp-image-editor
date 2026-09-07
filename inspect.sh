#!/usr/bin/env zsh

MAIN_FILE=$(find "/Users/saosun" -name "main.c" 2>/dev/null | head -n 1)
PROJ_DIR=$(dirname "$MAIN_FILE")
cd "$PROJ_DIR"

# Restore original clean source files if backups exist
[ -f gui.c.bak ] && cp gui.c.bak gui.c
[ -f image.c.bak ] && cp image.c.bak image.c
[ -f image.h.bak ] && cp image.h.bak image.h

echo "=== GUI.C OPEN CALLBACK ==="
grep -A 25 "open_image" gui.c 2>/dev/null || grep -A 25 "Open" gui.c 2>/dev/null

echo "\n=== IMAGE.C LOAD FUNCTION ==="
cat image.c
