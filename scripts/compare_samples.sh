#!/bin/bash
# Laesst unseren Compiler ueber die Beispielprogramme einer Blitz3D-Installation
# laufen - also ueber echten Fremdcode statt ueber unsere eigenen Tests. Das ist
# der schaerfste verfuegbare Test vor einem Release: unsere tests/ pruefen, was
# wir uns ausgedacht haben, diese Programme pruefen die Sprache.
#
# Verwendung:  bash scripts/compare_samples.sh [pfad/zu/Blitz3D]
# Vorgabe:     $BLITZ3D_HOME, sonst G:/dev/Blitz3D
#
# Die Kunst ist das Filtern, denn fast jedes dieser Programme benutzt 3D- oder
# Netzwerkbefehle, die wir gar nicht haben. Drei Stufen:
#
#   1. Unsere eigene Diagnose trennt die Klassen: ein fehlender Befehl meldet
#      "unknown function or command", alles andere ist ein Sprachbefund.
#      Programme, die AUSSCHLIESSLICH an unbekannten Befehlen scheitern, sind
#      eine Frage des Umfangs und werden getrennt gezaehlt.
#   2. Das Original ist die Gegenprobe. Viele Beispiele sind auch dort nicht
#      uebersetzbar (fehlende Includes, fehlende Medien) - was das Original
#      ablehnt, ist kein gueltiges Blitz3D und faellt heraus.
#   3. Der Rest wird nach der ERSTEN Meldung gruppiert, nicht nach Datei. Zehn
#      Programme, die an derselben Konstruktion scheitern, sind ein Befund.
#
# Am 2026-09-07 hat das aus 130 Programmen 49 echte Befunde herausgefiltert, und
# die gingen auf sechs Sprachkonstrukte zurueck (BUG-46 bis BUG-51).

set -u
B3D="${1:-${BLITZ3D_HOME:-G:/dev/Blitz3D}}"
ORIG="$B3D/bin/blitzcc.exe"
OURS="bin/blitzcc.exe"

[ -f "$OURS" ] || { echo "$OURS fehlt - erst bauen." >&2; exit 2; }
if [ ! -f "$ORIG" ]; then
    echo "Kein Original unter $ORIG - uebersprungen."
    exit 0
fi

export BLITZPATH="$B3D"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

i=0
while IFS= read -r f; do
    i=$((i + 1))
    cp "$f" "$TMP/$(printf 'p%03d' $i).bb" 2>/dev/null
    echo "$(printf 'p%03d' $i)|$f" >> "$TMP/index"
done < <(find "$B3D/samples" "$B3D/tutorials" "$B3D/Games" -iname "*.bb" 2>/dev/null)

total=$i
ok=0; scope=0; broken=0; real=0
: > "$TMP/findings"

for g in "$TMP"/p*.bb; do
    if "$OURS" -c "$g" >/dev/null 2>"$g.err"; then ok=$((ok + 1)); continue; fi

    # Stufe 1: nur unbekannte Befehle -> Umfang, kein Sprachbefund
    first=$(grep -i "error" "$g.err" | grep -v "unknown function or command" | head -1)
    if [ -z "$first" ]; then scope=$((scope + 1)); continue; fi

    # Stufe 2: was das Original selbst ablehnt, ist kein gueltiges Blitz3D
    if ! "$ORIG" -c "$g" >/dev/null 2>&1; then broken=$((broken + 1)); continue; fi

    real=$((real + 1))
    name=$(basename "$g" .bb)
    src=$(grep "^$name|" "$TMP/index" | head -1 | cut -d'|' -f2)
    msg=$(echo "$first" | sed 's#.*\.bb:##')
    line=$(echo "$msg" | cut -d: -f1)
    code=$(sed -n "${line}p" "$g" 2>/dev/null | sed 's/^[ \t]*//' | cut -c1-60)
    printf '%s\t%s\t%s\n' "$msg" "$code" "$src" >> "$TMP/findings"
done

echo "Beispielprogramme aus $B3D: $total"
echo
echo "  uebersetzt:                                   $ok"
echo "  scheitert nur an fehlenden Befehlen (Umfang): $scope"
echo "  auch vom Original abgelehnt (kaputt):         $broken"
echo "  ECHTE BEFUNDE (Original kann es, wir nicht):  $real"
echo
[ "$real" -eq 0 ] && { echo "Keine Sprachlücken gefunden."; exit 0; }

echo "== nach Meldung gruppiert =="
# Namen in Meldungen vereinheitlichen, damit gleiche Ursachen zusammenfallen
cut -f1 "$TMP/findings" | sed 's#^[0-9]*:[0-9]*: error: ##' \
    | sed "s/'[^']*'/'X'/g" | sort | uniq -c | sort -rn

echo
echo "== je Gruppe eine Beispielzeile =="
cut -f1,2 "$TMP/findings" | sed 's#^[0-9]*:[0-9]*: error: ##' \
    | sed "s/'[^']*'/'X'/" | sort -u -t"$(printf '\t')" -k1,1 \
    | while IFS=$(printf '\t') read -r m c; do printf '  %-34s | %s\n' "$m" "$c"; done
