#!/bin/bash

# Usage: ./build_release.sh battlefield_demo/crg_battlefield.cpp

if [ -z "$1" ]; then
    echo "[ERREUR] Spécifie le chemin du fichier."
    exit 1
fi

SOURCE="$1"
# On récupère le dossier où se trouve le fichier source
SOURCE_DIR=$(dirname "$SOURCE")
OUTNAME=$(basename "$SOURCE" .cpp)

echo "--- Compilation Release (Mac optimized) ---"

clang++ -O3 -DNDEBUG -std=c++20 "$SOURCE" -o "$SOURCE_DIR/$OUTNAME" \
  -I"$SOURCE_DIR" \
  -I/opt/homebrew/include \
  -I/usr/local/include \
  -L/opt/homebrew/lib \
  -L/usr/local/lib \
  -lraylib \
  -framework CoreVideo -framework IOKit -framework Cocoa -framework GLUT -framework OpenGL
  
if [ $? -eq 0 ]; then
    echo "[OK] Exécutable généré dans : $SOURCE_DIR/$OUTNAME"
else
    echo "[FAIL] Erreur de compilation."
fi