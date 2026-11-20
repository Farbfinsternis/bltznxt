#!/usr/bin/env bash
# ============================================================
# Adds the BLTZNXT IDE to the application menu (and the desktop).
#
#   install-desktop.sh              install
#   install-desktop.sh --user       only for this user (~/.local/share)
#   install-desktop.sh --system     for all users (/usr/local/share, uses sudo)
#   install-desktop.sh --no-desktop no icon on the desktop
#   install-desktop.sh --uninstall  remove everything this script installed
#
# Works from the Linux package (next to bltznxt-ide) and from the source tree
# (scripts/install-desktop.sh, starts ide/start-linux.sh).
#
# Without --user/--system it installs for this user - except under WSL: the
# Windows Start menu (WSLg) only lists entries in /usr/share/applications and
# /usr/local/share/applications, so there it installs system-wide and asks for
# the sudo password. The entry then shows up in the Windows Start menu as
# "BLTZNXT IDE (<distribution>)".
#
# The entry points to this folder: after moving the folder, run it again.
# ============================================================
set -euo pipefail

here="$(cd "$(dirname "$(readlink -f "${BASH_SOURCE[0]}")")" && pwd)"
IS_WSL=0
grep -qi microsoft /proc/version 2>/dev/null && IS_WSL=1
MODE=""
DESKTOP_ICON=1
UNINSTALL=0
for arg in "$@"; do
    case "$arg" in
        --user)       MODE=user ;;
        --system)     MODE=system ;;
        --no-desktop) DESKTOP_ICON=0 ;;
        --uninstall)  UNINSTALL=1 ;;
        -h|--help)    sed -n '2,21p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "unknown option: $arg" >&2; exit 2 ;;
    esac
done
[ -n "$MODE" ] || { [ "$IS_WSL" = 1 ] && MODE=system || MODE=user; }

ID=bltznxt-ide
USER_DATA="${XDG_DATA_HOME:-$HOME/.local/share}"
SYS_DATA=/usr/local/share
if [ "$MODE" = system ]; then DATA="$SYS_DATA"; else DATA="$USER_DATA"; fi
APPS="$DATA/applications"
ICONS="$DATA/icons/hicolor"
DESKTOP_DIR="$(xdg-user-dir DESKTOP 2>/dev/null || true)"
[ -n "$DESKTOP_DIR" ] || DESKTOP_DIR="$HOME/Desktop"
[ "$DESKTOP_DIR" != "$HOME" ] || DESKTOP_DIR=""   # xdg-user-dir answers $HOME when there is none

# Writing to /usr/local/share needs root: as root directly, otherwise via sudo
SUDO=()
if [ "$MODE" = system ] && [ "$(id -u)" != 0 ]; then
    command -v sudo >/dev/null 2>&1 || { echo "[ERROR] --system needs root, and sudo is missing" >&2; exit 1; }
    echo "Installing for all users into $SYS_DATA - sudo asks for your password."
    SUDO=(sudo)
fi
as_root() { "${SUDO[@]}" "$@"; }

refresh() {   # refresh <applications dir> <hicolor dir> <run as root 0|1>
    local r=(); [ "$3" = 1 ] && r=("${SUDO[@]}")
    command -v update-desktop-database >/dev/null 2>&1 && "${r[@]}" update-desktop-database -q "$1" 2>/dev/null || true
    command -v gtk-update-icon-cache  >/dev/null 2>&1 && "${r[@]}" gtk-update-icon-cache -q -t "$2" 2>/dev/null || true
}

remove_from() {   # remove_from <data dir> <run as root 0|1>
    local r=(); [ "$2" = 1 ] && r=("${SUDO[@]}")
    [ -e "$1/applications/$ID.desktop" ] || [ -e "$1/icons/hicolor/256x256/apps/$ID.png" ] || return 0
    "${r[@]}" rm -f "$1/applications/$ID.desktop"
    for s in 48 128 256; do "${r[@]}" rm -f "$1/icons/hicolor/${s}x${s}/apps/$ID.png"; done
    refresh "$1/applications" "$1/icons/hicolor" "$2"
    echo "Removed from $1"
}

if [ "$UNINSTALL" = 1 ]; then
    remove_from "$USER_DATA" 0
    if [ -e "$SYS_DATA/applications/$ID.desktop" ]; then
        if [ "$(id -u)" != 0 ] && [ ${#SUDO[@]} = 0 ]; then SUDO=(sudo); fi
        remove_from "$SYS_DATA" 1
    fi
    [ -n "$DESKTOP_DIR" ] && rm -f "$DESKTOP_DIR/$ID.desktop"
    echo "BLTZNXT IDE removed from the menu."
    exit 0
fi

# Package: bltznxt-ide next to this script, icons inside the Electron app.
# Source tree: this script lives in scripts/, the IDE in ide/.
if [ -x "$here/bltznxt-ide" ]; then
    EXEC="$here/bltznxt-ide"
    ICON_SRC="$here/ide/resources/app/icons"
    WORKDIR="$here"
elif [ -x "$here/../ide/start-linux.sh" ]; then
    root="$(cd "$here/.." && pwd)"
    EXEC="$root/ide/start-linux.sh"
    ICON_SRC="$root/ide/icons"
    WORKDIR="$root"
else
    echo "[ERROR] neither bltznxt-ide nor ide/start-linux.sh found next to this script" >&2
    exit 1
fi
[ -f "$ICON_SRC/bltznxt-256.png" ] || { echo "[ERROR] icon missing: $ICON_SRC/bltznxt-256.png" >&2; exit 1; }

for s in 48 128 256; do
    as_root install -D -m 644 "$ICON_SRC/bltznxt-$s.png" "$ICONS/${s}x${s}/apps/$ID.png"
done

# Exec quoted: the folder may contain spaces. %F hands files dropped on the
# icon to the IDE, which opens them. The icon as an absolute path: WSLg does
# not search every icon folder by name.
esc() { printf '%s' "$1" | sed 's/\\/\\\\\\\\/g; s/"/\\\\"/g; s/`/\\\\`/g; s/\$/\\\\$/g'; }
entry="$(mktemp)"
trap 'rm -f "$entry"' EXIT
cat > "$entry" <<EOF
[Desktop Entry]
Type=Application
Version=1.0
Name=BLTZNXT IDE
GenericName=Blitz3D IDE
GenericName[de]=Blitz3D-Entwicklungsumgebung
Comment=Write and run Blitz3D programs
Comment[de]=Blitz3D-Programme schreiben und starten
Exec="$(esc "$EXEC")" %F
Path=$WORKDIR
Icon=$ICONS/256x256/apps/$ID.png
Terminal=false
Categories=Development;IDE;
Keywords=Blitz3D;BlitzBasic;BASIC;game;
StartupWMClass=$ID
StartupNotify=true
EOF
as_root install -D -m 644 "$entry" "$APPS/$ID.desktop"
echo "Menu entry:   $APPS/$ID.desktop"
# One entry only: a system install replaces an earlier user install
[ "$MODE" = system ] && remove_from "$USER_DATA" 0

if [ "$DESKTOP_ICON" = 1 ] && [ -n "$DESKTOP_DIR" ] && [ -d "$DESKTOP_DIR" ]; then
    install -m 755 "$entry" "$DESKTOP_DIR/$ID.desktop"
    # GNOME and others start a desktop file only when it is executable and trusted
    command -v gio >/dev/null 2>&1 && gio set "$DESKTOP_DIR/$ID.desktop" metadata::trusted true 2>/dev/null || true
    echo "Desktop icon: $DESKTOP_DIR/$ID.desktop"
fi

refresh "$APPS" "$ICONS" "$([ "$MODE" = system ] && echo 1 || echo 0)"
echo "BLTZNXT IDE is in the application menu (category Development)."
if [ "$IS_WSL" = 1 ]; then
    if [ "$MODE" = system ]; then
        echo "WSL: it appears in the Windows Start menu as \"BLTZNXT IDE (${WSL_DISTRO_NAME:-Linux})\" within a few seconds."
    else
        echo "WSL: user entries do not reach the Windows Start menu - run again without --user for that."
    fi
fi
