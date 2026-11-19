#!/usr/bin/env bash
# ============================================================
# BLTZNXT — build setup for Linux
#
#   ./build_linux.sh               check dependencies, build SDL3 + SDL3_ttf, build blitzcc
#   ./build_linux.sh --setup-only  same, but stop before blitzcc
#   ./build_linux.sh --check       only check dependencies and print the install command
#   ./build_linux.sh --clean-deps  rebuild SDL3 + SDL3_ttf from scratch
#
# Unlike build_windows.bat, compiler and CMake come from the system: SDL has to
# be built against the system's display and audio libraries (X11/Wayland,
# PulseAudio/PipeWire), so a foreign prebuilt package would not fit anyway.
#
# SDL3 and SDL3_ttf are taken from the system when pkg-config finds the right
# version; otherwise their sources are downloaded (checksum-verified) and built
# into libs/linux/. Versions match the Windows packages in build_windows.bat.
# ============================================================
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIBS_DIR="$ROOT_DIR/libs/linux"
DEPS_BUILD="$ROOT_DIR/build/linux-deps"

SDL3_VERSION="3.2.0"
SDL3_URL="https://github.com/libsdl-org/SDL/releases/download/release-${SDL3_VERSION}/SDL3-${SDL3_VERSION}.tar.gz"
SDL3_SHA256="bf308f92c5688b1479faf5cfe24af72f3cd4ce08d0c0670d6ce55bc2ec1e9a5e"

TTF_VERSION="3.2.2"
TTF_URL="https://github.com/libsdl-org/SDL_ttf/releases/download/release-${TTF_VERSION}/SDL3_ttf-${TTF_VERSION}.tar.gz"
TTF_SHA256="63547d58d0185c833213885b635a2c0548201cc8f301e6587c0be1a67e1e045d"

MODE="all"
CLEAN_DEPS=0
for arg in "$@"; do
    case "$arg" in
        --setup-only) MODE="setup" ;;
        --check)      MODE="check" ;;
        --clean-deps) CLEAN_DEPS=1 ;;
        -h|--help)    sed -n '2,17p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "[ERROR] Unknown option: $arg (see --help)"; exit 2 ;;
    esac
done

info()  { echo "[INFO] $*"; }
warn()  { echo "[WARN] $*"; }
fail()  { echo "[ERROR] $*" >&2; exit 1; }

info "BLTZNXT build setup (Linux)"
info "Root: $ROOT_DIR"

# ------------------------------------------------------------
# 1. Dependencies
# ------------------------------------------------------------
# Package manager -> install command and package names.
PM=""
if   command -v apt-get >/dev/null 2>&1; then PM="apt"
elif command -v dnf     >/dev/null 2>&1; then PM="dnf"
elif command -v pacman  >/dev/null 2>&1; then PM="pacman"
elif command -v zypper  >/dev/null 2>&1; then PM="zypper"
fi

# Each entry: "<kind>:<name>:<required 1/0>:<apt>:<dnf>:<pacman>:<zypper>"
#   kind "cmd" = program in PATH, "pc" = pkg-config module
DEPS=(
    "cmd:g++:1:build-essential:gcc-c++:base-devel:gcc-c++"
    "cmd:make:1:build-essential:make:base-devel:make"
    "cmd:cmake:1:cmake:cmake:cmake:cmake"
    "cmd:pkg-config:1:pkg-config:pkgconf-pkg-config:pkgconf:pkg-config"
    "cmd:curl:1:curl:curl:curl:curl"
    "cmd:ninja:0:ninja-build:ninja-build:ninja:ninja"
    "pc:x11:1:libx11-dev:libX11-devel:libx11:libX11-devel"
    "pc:xext:1:libxext-dev:libXext-devel:libxext:libXext-devel"
    "pc:xrandr:1:libxrandr-dev:libXrandr-devel:libxrandr:libXrandr-devel"
    "pc:xcursor:1:libxcursor-dev:libXcursor-devel:libxcursor:libXcursor-devel"
    "pc:xfixes:1:libxfixes-dev:libXfixes-devel:libxfixes:libXfixes-devel"
    "pc:xi:1:libxi-dev:libXi-devel:libxi:libXi-devel"
    "pc:xscrnsaver:0:libxss-dev:libXScrnSaver-devel:libxss:libXss-devel"
    "pc:xtst:0:libxtst-dev:libXtst-devel:libxtst:libXtst-devel"
    "pc:xkbcommon:1:libxkbcommon-dev:libxkbcommon-devel:libxkbcommon:libxkbcommon-devel"
    "pc:wayland-client:1:libwayland-dev:wayland-devel:wayland:wayland-devel"
    "pc:wayland-protocols:1:wayland-protocols:wayland-protocols-devel:wayland-protocols:wayland-protocols-devel"
    "pc:libdecor-0:0:libdecor-0-dev:libdecor-devel:libdecor:libdecor-devel"
    "pc:gl:1:libgl1-mesa-dev:mesa-libGL-devel:mesa:Mesa-libGL-devel"
    "pc:egl:1:libegl1-mesa-dev:mesa-libEGL-devel:mesa:Mesa-libEGL-devel"
    "pc:glesv2:0:libgles2-mesa-dev:mesa-libGLES-devel:mesa:Mesa-libGLESv2-devel"
    "pc:libdrm:0:libdrm-dev:libdrm-devel:libdrm:libdrm-devel"
    "pc:gbm:0:libgbm-dev:mesa-libgbm-devel:mesa:libgbm-devel"
    "pc:alsa:1:libasound2-dev:alsa-lib-devel:alsa-lib:alsa-devel"
    "pc:libpulse:1:libpulse-dev:pulseaudio-libs-devel:libpulse:libpulse-devel"
    "pc:libpipewire-0.3:0:libpipewire-0.3-dev:pipewire-devel:libpipewire:pipewire-devel"
    "pc:dbus-1:0:libdbus-1-dev:dbus-devel:dbus:dbus-1-devel"
    "pc:libudev:0:libudev-dev:systemd-devel:systemd-libs:libudev-devel"
    "pc:ibus-1.0:0:libibus-1.0-dev:ibus-devel:ibus:ibus-devel"
    "pc:freetype2:1:libfreetype-dev:freetype-devel:freetype2:freetype2-devel"
    "pc:harfbuzz:1:libharfbuzz-dev:harfbuzz-devel:harfbuzz:harfbuzz-devel"
    "pc:fontconfig:1:libfontconfig-dev:fontconfig-devel:fontconfig:fontconfig-devel"
)

have_dep() {
    local kind="$1" name="$2"
    if [ "$kind" = "cmd" ]; then
        command -v "$name" >/dev/null 2>&1
    else
        command -v pkg-config >/dev/null 2>&1 && pkg-config --exists "$name"
    fi
}

pkg_for() {
    # $1 = entry; prints the package name for the detected package manager
    local IFS=':'; read -r _ _ _ p_apt p_dnf p_pacman p_zypper <<< "$1"
    case "$PM" in
        apt) echo "$p_apt" ;; dnf) echo "$p_dnf" ;;
        pacman) echo "$p_pacman" ;; zypper) echo "$p_zypper" ;;
    esac
}

MISSING_REQ=(); MISSING_OPT=(); PKGS_REQ=(); PKGS_OPT=()
for entry in "${DEPS[@]}"; do
    IFS=':' read -r kind name req _ <<< "$entry"
    if ! have_dep "$kind" "$name"; then
        pkg="$(pkg_for "$entry")"
        if [ "$req" = "1" ]; then
            MISSING_REQ+=("$name"); [ -n "$pkg" ] && PKGS_REQ+=("$pkg")
        else
            MISSING_OPT+=("$name"); [ -n "$pkg" ] && PKGS_OPT+=("$pkg")
        fi
    fi
done

uniq_words() { printf '%s\n' "$@" | awk 'NF && !seen[$0]++' | tr '\n' ' '; }

install_cmd() {
    local pkgs; pkgs="$(uniq_words "$@")"
    case "$PM" in
        apt)    echo "sudo apt-get install -y $pkgs" ;;
        dnf)    echo "sudo dnf install -y $pkgs" ;;
        pacman) echo "sudo pacman -S --needed $pkgs" ;;
        zypper) echo "sudo zypper install -y $pkgs" ;;
        *)      echo "(unknown package manager — install the development packages for: $pkgs)" ;;
    esac
}

if [ ${#MISSING_OPT[@]} -gt 0 ]; then
    warn "Optional, SDL works without them (fewer features): ${MISSING_OPT[*]}"
fi
if [ ${#MISSING_REQ[@]} -gt 0 ]; then
    echo
    echo "[ERROR] Missing required dependencies: ${MISSING_REQ[*]}"
    echo
    echo "Install them with (recommended: including the optional ones):"
    echo
    echo "    $(install_cmd "${PKGS_REQ[@]}" "${PKGS_OPT[@]}")"
    echo
    echo "Then run this script again."
    exit 1
fi
info "All required dependencies found."
if [ "$MODE" = "check" ]; then
    if [ ${#MISSING_OPT[@]} -gt 0 ]; then
        echo "    Optional: $(install_cmd "${PKGS_OPT[@]}")"
    fi
    exit 0
fi

CMAKE_VER="$(cmake --version | head -1 | awk '{print $3}')"
if [ "$(printf '%s\n3.16\n' "$CMAKE_VER" | sort -V | head -1)" != "3.16" ]; then
    fail "CMake $CMAKE_VER is too old, SDL3 needs 3.16 or newer."
fi

GENERATOR=(-G "Unix Makefiles")
command -v ninja >/dev/null 2>&1 && GENERATOR=(-G Ninja)
JOBS="$(nproc 2>/dev/null || echo 4)"

# ------------------------------------------------------------
# 2. SDL3 + SDL3_ttf
# ------------------------------------------------------------
fetch() {
    # fetch <url> <sha256> <target file>
    local url="$1" sum="$2" out="$3"
    if [ -f "$out" ] && echo "$sum  $out" | sha256sum -c --status; then
        return 0
    fi
    info "Downloading $url"
    curl -fL --retry 3 -o "$out.part" "$url"
    if ! echo "$sum  $out.part" | sha256sum -c --status; then
        rm -f "$out.part"
        fail "Checksum mismatch for $url — download corrupt or tampered with."
    fi
    mv "$out.part" "$out"
}

# system_has <pc module> <min version>
system_has() { pkg-config --exists "$1 >= $2" 2>/dev/null; }

if [ "$CLEAN_DEPS" = "1" ]; then
    info "Removing previously built SDL3 / SDL3_ttf"
    rm -rf "$LIBS_DIR" "$DEPS_BUILD"
fi
mkdir -p "$DEPS_BUILD" "$LIBS_DIR"

SDL3_PREFIX="$LIBS_DIR/sdl3"
TTF_PREFIX="$LIBS_DIR/sdl3_ttf"

if system_has sdl3 "$SDL3_VERSION"; then
    SDL3_PREFIX=""
    info "SDL3 $(pkg-config --modversion sdl3) from the system"
elif [ -f "$SDL3_PREFIX/lib/cmake/SDL3/SDL3Config.cmake" ]; then
    info "SDL3 found at $SDL3_PREFIX"
else
    info "Building SDL3 $SDL3_VERSION (takes a few minutes)"
    fetch "$SDL3_URL" "$SDL3_SHA256" "$DEPS_BUILD/SDL3-$SDL3_VERSION.tar.gz"
    rm -rf "$DEPS_BUILD/SDL3-$SDL3_VERSION" "$DEPS_BUILD/sdl3-build"
    tar -xzf "$DEPS_BUILD/SDL3-$SDL3_VERSION.tar.gz" -C "$DEPS_BUILD"
    cmake -S "$DEPS_BUILD/SDL3-$SDL3_VERSION" -B "$DEPS_BUILD/sdl3-build" "${GENERATOR[@]}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$SDL3_PREFIX" \
        -DCMAKE_INSTALL_LIBDIR=lib \
        -DSDL_SHARED=ON -DSDL_STATIC=OFF \
        -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF
    cmake --build "$DEPS_BUILD/sdl3-build" -j "$JOBS"
    cmake --install "$DEPS_BUILD/sdl3-build"
    info "SDL3 installed to $SDL3_PREFIX"
fi

if [ -z "$SDL3_PREFIX" ] && system_has sdl3-ttf "$TTF_VERSION"; then
    TTF_PREFIX=""
    info "SDL3_ttf $(pkg-config --modversion sdl3-ttf) from the system"
elif [ -f "$TTF_PREFIX/lib/cmake/SDL3_ttf/SDL3_ttfConfig.cmake" ]; then
    info "SDL3_ttf found at $TTF_PREFIX"
else
    info "Building SDL3_ttf $TTF_VERSION"
    fetch "$TTF_URL" "$TTF_SHA256" "$DEPS_BUILD/SDL3_ttf-$TTF_VERSION.tar.gz"
    rm -rf "$DEPS_BUILD/SDL3_ttf-$TTF_VERSION" "$DEPS_BUILD/ttf-build"
    tar -xzf "$DEPS_BUILD/SDL3_ttf-$TTF_VERSION.tar.gz" -C "$DEPS_BUILD"
    PREFIX_PATH="$SDL3_PREFIX"
    [ -z "$PREFIX_PATH" ] && PREFIX_PATH="/usr"
    cmake -S "$DEPS_BUILD/SDL3_ttf-$TTF_VERSION" -B "$DEPS_BUILD/ttf-build" "${GENERATOR[@]}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$TTF_PREFIX" \
        -DCMAKE_INSTALL_LIBDIR=lib \
        -DCMAKE_PREFIX_PATH="$PREFIX_PATH" \
        -DBUILD_SHARED_LIBS=ON \
        -DSDLTTF_VENDORED=OFF \
        -DSDLTTF_HARFBUZZ=ON \
        -DSDLTTF_PLUTOSVG=OFF \
        -DSDLTTF_SAMPLES=OFF
    cmake --build "$DEPS_BUILD/ttf-build" -j "$JOBS"
    cmake --install "$DEPS_BUILD/ttf-build"
    info "SDL3_ttf installed to $TTF_PREFIX"
fi

if [ "$MODE" = "setup" ]; then
    info "Setup complete. Skipping blitzcc build as requested."
    exit 0
fi

# ------------------------------------------------------------
# 3. blitzcc
# ------------------------------------------------------------
info "Building blitzcc"
CMAKE_ARGS=(-DCMAKE_BUILD_TYPE=Release)
[ -n "$SDL3_PREFIX" ] && CMAKE_ARGS+=(-DSDL3_DIR="$SDL3_PREFIX/lib/cmake/SDL3")
cmake -S "$ROOT_DIR" -B "$ROOT_DIR/build/linux" "${GENERATOR[@]}" "${CMAKE_ARGS[@]}"
cmake --build "$ROOT_DIR/build/linux" -j "$JOBS"

info "Build finished: $ROOT_DIR/bin/blitzcc"
