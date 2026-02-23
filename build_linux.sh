#!/bin/bash
set -e

# --- Configuration ---
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOOLS_DIR="$ROOT_DIR/tools"
LIBS_DIR="$ROOT_DIR/libs"

echo "[INFO] BlitzNext Toolchain Setup"
echo "[INFO] Root: $ROOT_DIR"

mkdir -p "$TOOLS_DIR"
mkdir -p "$LIBS_DIR"

# --- 1. Portable GCC (14.2.0) ---
GCC_DIR="$TOOLS_DIR/gcc"
GCC_URL="https://github.com/tttapa/toolchains/releases/download/v14.2.0-linux-x64/x86_64-linux-gnu-gcc-14.2.0.tar.xz"
GCC_TAR="$TOOLS_DIR/gcc.tar.xz"

if [ ! -d "$GCC_DIR" ]; then
    echo "[INFO] Portable GCC not found. Downloading..."
    curl -L "$GCC_URL" -o "$GCC_TAR"
    echo "[INFO] Extracting GCC..."
    mkdir -p "$GCC_DIR"
    tar -xf "$GCC_TAR" -C "$GCC_DIR" --strip-components=1
    rm "$GCC_TAR"
    echo "[INFO] GCC setup complete."
else
    echo "[INFO] GCC found at $GCC_DIR"
fi

# --- 2. CMake (3.31.5) ---
CMAKE_DIR="$TOOLS_DIR/cmake"
CMAKE_URL="https://github.com/Kitware/CMake/releases/download/v3.31.5/cmake-3.31.5-linux-x86_64.tar.gz"
CMAKE_TAR="$TOOLS_DIR/cmake.tar.gz"

if [ ! -d "$CMAKE_DIR" ]; then
    echo "[INFO] CMake not found. Downloading..."
    curl -L "$CMAKE_URL" -o "$CMAKE_TAR"
    echo "[INFO] Extracting CMake..."
    mkdir -p "$CMAKE_DIR"
    tar -xf "$CMAKE_TAR" -C "$CMAKE_DIR" --strip-components=1
    rm "$CMAKE_TAR"
    echo "[INFO] CMake setup complete."
else
    echo "[INFO] CMake found at $CMAKE_DIR"
fi

# --- 3. SDL3 (Development Libraries) ---
SDL_DIR="$LIBS_DIR/sd3"
SDL_URL="https://github.com/libsdl-org/SDL/releases/download/release-3.2.0/SDL3-3.2.0.tar.gz"
SDL_TAR="$LIBS_DIR/sdl3.tar.gz"

if [ ! -d "$SDL_DIR" ]; then
    echo "[INFO] SDL3 not found. Downloading..."
    curl -L "$SDL_URL" -o "$SDL_TAR"
    echo "[INFO] Extracting SDL3..."
    mkdir -p "$SDL_DIR"
    tar -xf "$SDL_TAR" -C "$SDL_DIR" --strip-components=1
    rm "$SDL_TAR"
    echo "[INFO] SDL3 setup complete."
else
    echo "[INFO] SDL3 found at $SDL_DIR"
fi

# --- 4. Build BlitzNext ---
echo "[INFO] Starting BlitzNext build..."
export PATH="$GCC_DIR/bin:$CMAKE_DIR/bin:$PATH"

mkdir -p build
cd build
cmake .. -DSDL3_DIR="$SDL_DIR" -DJSON_INCLUDE_DIR="$TOOLS_DIR"
make
cd ..

echo "[INFO] Build finished."
