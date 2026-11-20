#!/usr/bin/env bash
# ============================================================
# Builds the Linux package of BLTZNXT: compiler, IDE, SDL3, samples.
# Counterpart of scripts/package_release.py (Windows).
#
#   scripts/package_linux.sh            dist/bltznxt-v<version>-linux-x64/ and .tar.gz
#   scripts/package_linux.sh --verify   then check the package on its own
#   scripts/package_linux.sh --no-build do not rebuild ide/dist
#
# Requirements: ./build_linux.sh has run (bin/blitzcc, libs/linux/sdl3,
# libs/linux/sdl3_ttf), ide/ has node_modules (npm install).
# The version is in ide/package.json; blitzcc must report the same (-v).
#
# Layout:
#   bltznxt-v0.7.2-linux-x64/
#     bltznxt-ide            starts the IDE
#     install-desktop.sh     menu entry and desktop icon for the IDE
#     README.txt  LICENSES/
#     bin/blitzcc
#     ide/                   Electron with the IDE in resources/app
#     src/compiler/          runtime headers the generated C++ includes
#     src/thirdparty/        stb, dr_mp3, ...
#     libs/linux/            SDL3, SDL3_ttf (headers and shared libraries)
#     samples/friendlyfire/  the sample game
#     examples/              more examples
#
# Unlike the Windows package there is no C++ compiler inside: blitzcc uses the
# system's g++ (build-essential, gcc-c++, base-devel).
# ============================================================
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD=1
VERIFY=0
for arg in "$@"; do
    case "$arg" in
        --no-build) BUILD=0 ;;
        --verify)   VERIFY=1 ;;
        -h|--help)  sed -n '2,30p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "unknown option: $arg" >&2; exit 2 ;;
    esac
done

log()  { echo "$*"; }
fail() { echo "[ERROR] $*" >&2; exit 1; }

VER="$(sed -n 's/.*"version": *"\([^"]*\)".*/\1/p' "$ROOT/ide/package.json" | head -1)"
[ -x "$ROOT/bin/blitzcc" ] || fail "bin/blitzcc missing - run ./build_linux.sh first"
REPORTED="$("$ROOT/bin/blitzcc" -v)"
[[ "$REPORTED" == *"$VER"* ]] || fail "bin/blitzcc reports '$REPORTED', expected $VER - rebuild it"
for lib in sdl3 sdl3_ttf; do
    [ -d "$ROOT/libs/linux/$lib/lib" ] || fail "libs/linux/$lib missing - run ./build_linux.sh first"
done
[ -x "$ROOT/ide/node_modules/electron/dist/electron" ] || fail "ide/node_modules missing - run 'npm install' in ide/"

NAME="bltznxt-v$VER-linux-x64"
OUT="$ROOT/dist/$NAME"
rm -rf "$OUT"
mkdir -p "$OUT"

# The version is part of the built renderer ("About"): build it fresh
if [ "$BUILD" = 1 ]; then
    log "... vite build (ide)"
    (cd "$ROOT/ide" && npx vite build --logLevel warn)
fi
[ -f "$ROOT/ide/dist/index.html" ] || fail "ide/dist missing - run 'npx vite build' in ide/"

log "... compiler"
mkdir -p "$OUT/bin" "$OUT/src/compiler"
cp "$ROOT/bin/blitzcc" "$OUT/bin/"
strip "$OUT/bin/blitzcc" 2>/dev/null || true
cp "$ROOT"/src/compiler/*.h "$OUT/src/compiler/"
cp -r "$ROOT/src/thirdparty" "$OUT/src/"

log "... libraries"
for lib in sdl3 sdl3_ttf; do
    mkdir -p "$OUT/libs/linux/$lib/lib"
    cp -r "$ROOT/libs/linux/$lib/include" "$OUT/libs/linux/$lib/"
    # only the shared libraries, symlinks kept
    cp -a "$ROOT/libs/linux/$lib/lib/"*.so* "$OUT/libs/linux/$lib/lib/"
done

log "... ide"
cp -r "$ROOT/ide/node_modules/electron/dist" "$OUT/ide"
find "$OUT/ide/locales" -name '*.pak' ! -name 'en-US.pak' ! -name 'de.pak' -delete
rm -f "$OUT/ide/resources/default_app.asar"
mv "$OUT/ide/electron" "$OUT/ide/bltznxt-ide"
APP="$OUT/ide/resources/app"
mkdir -p "$APP"
cp -r "$ROOT/ide/dist" "$ROOT/ide/electron" "$ROOT/ide/icons" "$APP/"
cp "$ROOT/ide/electron-main.js" "$APP/"
cat > "$APP/package.json" <<EOF
{
  "name": "bltznxt-ide",
  "version": "$VER",
  "main": "electron-main.js",
  "description": "BLTZNXT IDE"
}
EOF

log "... samples"
mkdir -p "$OUT/samples"
rsync -a --exclude '*.exe' --exclude '*.dll' --exclude 'kenney' --exclude 'roh' --exclude 'astra' \
      --exclude '__pycache__' --exclude '*.pyc' "$ROOT/samples/friendlyfire" "$OUT/samples/"
if [ -d "$ROOT/examples" ]; then
    rsync -a --exclude '*.exe' --exclude '*.dll' --exclude '*.cpp' --exclude '__pycache__' "$ROOT/examples" "$OUT/"
fi

log "... docs"
mkdir -p "$OUT/LICENSES"
cp "$ROOT/libs/linux/sdl3/share/licenses/SDL3/LICENSE.txt" "$OUT/LICENSES/SDL3-LICENSE.txt" 2>/dev/null ||
    cp "$ROOT/build/linux-deps/SDL3-"*/LICENSE.txt "$OUT/LICENSES/SDL3-LICENSE.txt"
cp "$ROOT/libs/linux/sdl3_ttf/share/licenses/SDL3_ttf/LICENSE.txt" "$OUT/LICENSES/SDL3_ttf-LICENSE.txt" 2>/dev/null ||
    cp "$ROOT/build/linux-deps/SDL3_ttf-"*/LICENSE.txt "$OUT/LICENSES/SDL3_ttf-LICENSE.txt"
cp "$ROOT/ide/node_modules/electron/dist/LICENSE" "$OUT/LICENSES/Electron-LICENSE.txt"
if [ -f "$ROOT/ide/node_modules/monaco-editor/LICENSE" ]; then
    cp "$ROOT/ide/node_modules/monaco-editor/LICENSE" "$OUT/LICENSES/monaco-editor-LICENSE.txt"
fi
sed "s/@VERSION@/$VER/g" "$ROOT/scripts/package_readme_linux.txt" > "$OUT/README.txt"

# Starter: Chromium's sandbox needs a setuid-root helper, which an unpacked
# archive cannot have - then the sandbox is switched off (both settings are
# needed, see ide/start-linux.sh).
cat > "$OUT/bltznxt-ide" <<'EOF'
#!/usr/bin/env bash
# Starts the BLTZNXT IDE. Files given on the command line are opened.
here="$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")" && pwd)"
ide="$here/ide/bltznxt-ide"
helper="$here/ide/chrome-sandbox"
if [ ! -u "$helper" ] || [ "$(stat -c %u "$helper")" != "0" ]; then
    export ELECTRON_DISABLE_SANDBOX=1
    exec "$ide" --no-sandbox "$@"
fi
exec "$ide" "$@"
EOF
chmod +x "$OUT/bltznxt-ide"
cp "$ROOT/scripts/install-desktop.sh" "$OUT/install-desktop.sh"
chmod +x "$OUT/install-desktop.sh"

log "Folder: $OUT ($(du -sh "$OUT" | cut -f1))"

# ---------------------------------------------------------------------------
FAILURES=0
check() {   # check <label> <0|1 ok> [detail]
    if [ "$2" = 1 ]; then echo "PASS  $1${3:+  -> $3}"; else echo "FAIL  $1${3:+  -> $3}"; FAILURES=$((FAILURES + 1)); fi
}
if [ "$VERIFY" = 1 ]; then
    # The package on its own: a clean folder, no BLITZPATH, the repository not reachable
    work="$(mktemp -d)"
    cp -r "$OUT" "$work/pkg"
    P="$work/pkg"
    run() { (cd "$work" && env -u BLITZPATH -u blitzide PATH=/usr/bin:/bin "$@"); }
    r="$(run "$P/bin/blitzcc" -v || true)"
    check "blitzcc -v reports the version" "$([[ "$r" == *"$VER"* ]] && echo 1 || echo 0)" "$r"
    printf 'Print "Hallo aus dem Paket"\r\nEnd\r\n' > "$work/text.bb"
    r="$(run "$P/bin/blitzcc" -q text.bb 2>&1 || true)"
    check "text program compiles" "$([ -x "$work/text" ] && echo 1 || echo 0)" "${r:0:300}"
    if [ -x "$work/text" ]; then
        r="$(run ./text </dev/null 2>&1 || true)"
        check "text program runs" "$([[ "$r" == *"Hallo aus dem Paket"* ]] && echo 1 || echo 0)" "$r"
    fi
    printf 'Graphics3D 320,240,32,2\r\nc = CreateCamera()\r\ncube = CreateCube()\r\nMoveEntity cube,0,0,5\r\nRenderWorld\r\nFlip\r\nEnd\r\n' > "$work/grafik.bb"
    r="$(run "$P/bin/blitzcc" -q grafik.bb 2>&1 || true)"
    check "3D program compiles and links SDL3" "$([ -x "$work/grafik" ] && echo 1 || echo 0)" "${r:0:300}"
    check "libSDL3.so.0 lies next to the program" "$([ -f "$work/libSDL3.so.0" ] && echo 1 || echo 0)"
    if [ -x "$work/grafik" ]; then
        rc=0; SDL_VIDEO_DRIVER=offscreen run ./grafik </dev/null >/dev/null 2>&1 || rc=$?
        check "3D program runs (offscreen)" "$([ $rc = 0 ] && echo 1 || echo 0)" "exit $rc"
    fi
    lt="$P/samples/friendlyfire/friendlyfire.bb"
    if [ -f "$lt" ]; then
        r="$(cd "$(dirname "$lt")" && env -u BLITZPATH PATH=/usr/bin:/bin blitzide=1 "$P/bin/blitzcc" -q -o "$work/friendlyfire" "$lt" 2>&1 || true)"
        check "Friendly Fire becomes a program (IDE protocol)" "$([ -x "$work/friendlyfire" ] && echo 1 || echo 0)" "$(echo "$r" | tr '\n' '|' | tail -c 300)"
    fi
    # Menu entry in a throw-away home: points to the package, icon installed
    fakehome="$work/home"; mkdir -p "$fakehome"
    r="$(HOME="$fakehome" XDG_DATA_HOME="$fakehome/.local/share" "$P/install-desktop.sh" --user --no-desktop 2>&1 || true)"
    de="$fakehome/.local/share/applications/bltznxt-ide.desktop"
    ok=0
    if [ -f "$de" ] && grep -q "^Exec=\"$P/bltznxt-ide\" %F$" "$de" \
       && [ -f "$fakehome/.local/share/icons/hicolor/256x256/apps/bltznxt-ide.png" ]; then ok=1; fi
    if [ "$ok" = 1 ] && command -v desktop-file-validate >/dev/null 2>&1; then
        desktop-file-validate "$de" >/dev/null 2>&1 || ok=0
    fi
    check "install-desktop.sh writes a menu entry for the package" "$ok" "$(echo "$r" | tr '\n' '|' | tail -c 300)"
    # The IDE through the starter: still running after 12 s means it came up
    # (a missing library or a sandbox problem ends it within a second)
    rc=0; (cd "$work" && timeout 12 "$P/bltznxt-ide" > "$work/ide.log" 2>&1) || rc=$?
    check "IDE starts through bltznxt-ide (still running after 12 s)" "$([ $rc = 124 ] && echo 1 || echo 0)" \
          "exit $rc $(grep -m1 -E 'FATAL|error while loading' "$work/ide.log" || true)"
    rm -rf "$work"
fi

log "... tar.gz"
TARGET="$ROOT/dist/$NAME.tar.gz"
rm -f "$TARGET"
tar -C "$ROOT/dist" -czf "$TARGET" "$NAME"
(cd "$ROOT/dist" && sha256sum "$NAME.tar.gz" > "$NAME.tar.gz.sha256")
log "Archive: $TARGET ($(du -h "$TARGET" | cut -f1))"
log "SHA-256: $(cut -d' ' -f1 "$TARGET.sha256")"
[ "$FAILURES" = 0 ]
