#!/bin/bash
# Misst die Kette "Frontend nimmt an" -> "erzeugtes C++ uebersetzt" ueber die
# Beispielprogramme einer Blitz3D-Installation.
#
# compare_samples.sh endet bei der ersten Stufe: "-c" heisst bei uns nur, dass
# C++ geschrieben wurde. Ob dieses C++ ueberhaupt uebersetzt, sagt es nicht -
# und genau dort lag die groesste Luecke (2026-09-09: 59 angenommen, 40
# uebersetzten). Ein voller "-static"-Bau dauert Minuten je Datei; die reine
# Syntaxpruefung des Emittats dauert Sekunden und beantwortet dieselbe Frage.
#
# Verwendung:  bash scripts/compile_samples.sh [pfad/zu/Blitz3D]
# Ausgabe:     Zaehler, danach je Fehlschlag die erste g++-Meldung.

set -u
B3D="${1:-${BLITZ3D_HOME:-G:/dev/Blitz3D}}"
OURS="bin/blitzcc.exe"
GPP="tools/mingw64/bin/g++.exe"

[ -f "$OURS" ] || { echo "$OURS fehlt - erst bauen." >&2; exit 2; }
[ -f "$GPP" ]  || { echo "$GPP fehlt." >&2; exit 2; }

OURS=$(cd "$(dirname "$OURS")" && pwd)/$(basename "$OURS")
GPP=$(cd "$(dirname "$GPP")" && pwd)/$(basename "$GPP")
INC=$(cd src/compiler && pwd)
SDLINC=$(cd libs/sd3/x86_64-w64-mingw32/include && pwd)
TTFINC=""
[ -d libs/sdl3_ttf/x86_64-w64-mingw32/include ] &&
    TTFINC=$(cd libs/sdl3_ttf/x86_64-w64-mingw32/include && pwd)

export BLITZPATH="$B3D"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

total=0; frontend=0; cpp_ok=0; cpp_fail=0
: > "$TMP/fails"

while IFS= read -r f; do
    total=$((total + 1))
    dir=$(dirname "$f")
    cpp="$TMP/out.cpp"
    rm -f "$cpp"
    ( cd "$dir" && "$OURS" -c -o "$TMP/out" "$f" ) >/dev/null 2>&1 || continue
    [ -f "$cpp" ] || continue
    frontend=$((frontend + 1))

    args=(-std=c++17 -fsyntax-only -I"$INC" -I"$SDLINC")
    [ -n "$TTFINC" ] && args+=(-I"$TTFINC" -DBB_HAS_SDL3_TTF)
    if "$GPP" "${args[@]}" "$cpp" >"$TMP/gpp" 2>&1; then
        cpp_ok=$((cpp_ok + 1))
    else
        cpp_fail=$((cpp_fail + 1))
        first=$(grep -m1 " error: " "$TMP/gpp")
        printf '%s\t%s\n' "${first#*error: }" "$f" >> "$TMP/fails"
    fi
done < <(find "$B3D/samples" "$B3D/tutorials" "$B3D/Games" -iname "*.bb" 2>/dev/null | sort)

echo "Beispielprogramme aus $B3D: $total"
echo "  Frontend nimmt an:            $frontend"
echo "  erzeugtes C++ uebersetzt:     $cpp_ok"
echo "  erzeugtes C++ uebersetzt NICHT: $cpp_fail"
[ "$cpp_fail" -eq 0 ] && exit 0

echo
echo "== nach g++-Meldung gruppiert =="
cut -f1 "$TMP/fails" | sed "s/'[^']*'/'X'/g" | sort | uniq -c | sort -rn
echo
echo "== je Fehlschlag =="
cat "$TMP/fails"
