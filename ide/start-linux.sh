#!/usr/bin/env bash
# ============================================================
# Start the BLTZNXT IDE on Linux from the source tree.
#
#   ide/start-linux.sh [file.bb ...]
#
# Needs Node.js 18 or newer with npm, and the compiler built by
# ../build_linux.sh (the IDE finds ../bin/blitzcc by itself).
# On the first run it installs the npm packages (Electron) and builds the
# user interface; later it rebuilds only when files in src/ changed.
# ============================================================
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

command -v npm >/dev/null 2>&1 || { echo "[ERROR] npm not found - install Node.js 18 or newer"; exit 1; }

if [ ! -x node_modules/.bin/electron ]; then
    echo "[INFO] Installing npm packages (Electron, Vite) ..."
    npm install --no-audit --no-fund
fi

if [ ! -f dist/index.html ] || [ -n "$(find src index.html vite.config.js -newer dist/index.html -print -quit)" ]; then
    echo "[INFO] Building the user interface ..."
    npx vite build --logLevel warn
fi

# Chromium's sandbox needs its helper owned by root with the setuid bit:
#   sudo chown root node_modules/electron/dist/chrome-sandbox
#   sudo chmod 4755 node_modules/electron/dist/chrome-sandbox
# Without that Electron refuses to start, so the sandbox is switched off. The
# IDE only loads its own files, and the renderer has no Node access
# (contextIsolation), so this is acceptable for a development start.
SANDBOX=()
helper=node_modules/electron/dist/chrome-sandbox
if [ ! -u "$helper" ] || [ "$(stat -c %u "$helper")" != "0" ]; then
    # --no-sandbox alone is not enough: without the variable the GPU process
    # still fails to start ("GPU process isn't usable") and Electron quits.
    export ELECTRON_DISABLE_SANDBOX=1
    SANDBOX=(--no-sandbox)
fi

# The binary itself, not the npm wrapper: signals (Ctrl+C, kill) reach it
exec node_modules/electron/dist/electron "${SANDBOX[@]}" . "$@"
