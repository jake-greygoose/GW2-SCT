#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$REPO_ROOT/build"
TOOLCHAIN_FILE="${TOOLCHAIN_FILE:-$REPO_ROOT/cmake/toolchains/mingw-w64.cmake}"
DIST_DIR="$REPO_ROOT/dist"

cmake -S "$REPO_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE"
cmake --build "$BUILD_DIR" --config Release

DLL_PATH="$BUILD_DIR/Release/Detarget.dll"
if [[ ! -f "$DLL_PATH" ]]; then
    DLL_PATH="$BUILD_DIR/Detarget.dll"
fi
if [[ ! -f "$DLL_PATH" ]]; then
    echo "Detarget.dll not found in build output." >&2
    exit 1
fi
BIN_DIR="$(dirname "$DLL_PATH")"
PDB_PATH="$BIN_DIR/Detarget.pdb"
if [[ ! -f "$PDB_PATH" ]]; then
    echo "Detarget.pdb not found next to $DLL_PATH." >&2
    exit 1
fi

rm -rf "$DIST_DIR"
mkdir -p "$DIST_DIR"
cp "$DLL_PATH" "$DIST_DIR"
cp "$PDB_PATH" "$DIST_DIR"

echo "Build completed. Artifacts copied to $DIST_DIR"
