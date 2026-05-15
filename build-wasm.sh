#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/compiler/build"
OUT_DIR="$SCRIPT_DIR/frontend/public/wasm"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
make -j"$(nproc)"

mkdir -p "$OUT_DIR"
cp src/LibreFractals.js src/LibreFractals.wasm "$OUT_DIR/"

echo "WASM build done. Files copied to $OUT_DIR"
