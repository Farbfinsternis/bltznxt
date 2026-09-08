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
#   2. Das Original ist die Gegenprobe. Manche Beispiele sind auch dort nicht
#      uebersetzbar (fehlende Medien, Bausteine groesserer Projekte) - was das
#      Original ablehnt, ist kein gueltiges Blitz3D und faellt heraus.
#   3. Der Rest wird nach der ERSTEN Meldung gruppiert, nicht nach Datei. Zehn
#      Programme, die an derselben Konstruktion scheitern, sind ein Befund.
#
# WICHTIG - Originalpfade bleiben erhalten (repariert am 2026-09-08):
# Bis dahin kopierte dieses Skript jede Quelle flach unter neuem Namen in ein
# temporaeres Verzeichnis. Damit gingen die relativen Include-Beziehungen
# verloren; weil Stufe 2 das Original auf derselben Kopie prueft, fiel jedes
# Programm mit Include dort als "kaputt" heraus, obwohl es an seinem
# Originalpfad uebersetzt. Die Messung unterschaetzte die Befunde dadurch
# erheblich: 49 statt 89 Dateien mit Sprachbefunden.
#
# Jetzt wird jede Datei an ihrem Platz uebersetzt, mit dem Arbeitsverzeichnis
# auf ihrem eigenen Ordner - nur unsere Ausgaben liegen im temporaeren
# Verzeichnis. Weder Beispiele noch Includes werden fuer die Messung angefasst.
#
# Was diese Messung NICHT beweist: "-c" bedeutet bei uns nur "C++ geschrieben",
# beim Original bereits Codegenerierung und Assemblierung. Ein beidseitiges "OK"
# ist also ein Frontend-Ergebnis, keine Zusage, dass das erzeugte C++ uebersetzt
# oder das Programm gleich laeuft. Am 2026-09-08 bestanden von 24 solchen
# Erfolgen 22 zusaetzlich die C++-Uebersetzung, 2 nicht.
#
# Referenzwert nach der Reparatur (2026-09-08, Commit 7605d7f + sechs Fixes):
# 130 Dateien, davon 24 Frontend-OK, 40 nur Umfang, 5 auch im Original
# abgelehnt, 61 echte Befunde. Diese 61 decken sich mit einer unabhaengigen
# Messung ueber dieselben Dateien - das ist die Gegenprobe fuer die Reparatur.

set -u
B3D="${1:-${BLITZ3D_HOME:-G:/dev/Blitz3D}}"
ORIG="$B3D/bin/blitzcc.exe"
OURS="bin/blitzcc.exe"

[ -f "$OURS" ] || { echo "$OURS fehlt - erst bauen." >&2; exit 2; }
if [ ! -f "$ORIG" ]; then
    echo "Kein Original unter $ORIG - uebersprungen."
    exit 0
fi

# Absolut machen: unten wird ins Verzeichnis der jeweiligen Quelle gewechselt,
# damit beide Compiler deren Includes so aufloesen wie im echten Projekt.
OURS=$(cd "$(dirname "$OURS")" && pwd)/$(basename "$OURS")
ORIG=$(cd "$(dirname "$ORIG")" && pwd)/$(basename "$ORIG")

export BLITZPATH="$B3D"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

total=0; ok=0; scope=0; broken=0; real=0
: > "$TMP/findings"

while IFS= read -r f; do
    total=$((total + 1))
    dir=$(dirname "$f")
    err="$TMP/err"

    # Stufe 1: unsere Diagnose. Ausgabe ins TMP, Quelle bleibt unberuehrt.
    if ( cd "$dir" && "$OURS" -c -o "$TMP/out" "$f" ) >/dev/null 2>"$err"; then
        ok=$((ok + 1)); continue
    fi

    first=$(grep -i "error" "$err" | grep -v "unknown function or command" | head -1)
    if [ -z "$first" ]; then scope=$((scope + 1)); continue; fi

    # Stufe 2: das Original auf DERSELBEN Datei an ihrem Originalpfad.
    if ! ( cd "$dir" && "$ORIG" -c "$f" ) >/dev/null 2>&1; then
        broken=$((broken + 1)); continue
    fi

    real=$((real + 1))
    msg=$(echo "$first" | sed 's#.*\.bb:##')
    line=$(echo "$msg" | cut -d: -f1)
    code=$(sed -n "${line}p" "$f" 2>/dev/null | sed 's/^[ \t]*//' | cut -c1-60)
    printf '%s\t%s\t%s\n' "$msg" "$code" "$f" >> "$TMP/findings"
done < <(find "$B3D/samples" "$B3D/tutorials" "$B3D/Games" -iname "*.bb" 2>/dev/null | sort)

echo "Beispielprogramme aus $B3D: $total"
echo
echo "  Frontend-OK bei uns:                          $ok"
echo "  scheitert nur an fehlenden Befehlen (Umfang): $scope"
echo "  auch vom Original abgelehnt (kein Blitz3D):   $broken"
echo "  ECHTE BEFUNDE (Original kann es, wir nicht):  $real"
echo
[ "$real" -eq 0 ] && { echo "Keine Sprachluecken gefunden."; exit 0; }

echo "== nach Meldung gruppiert =="
# Namen in Meldungen vereinheitlichen, damit gleiche Ursachen zusammenfallen
cut -f1 "$TMP/findings" | sed 's#^[0-9]*:[0-9]*: error: ##' \
    | sed "s/'[^']*'/'X'/g" | sort | uniq -c | sort -rn

echo
echo "== je Gruppe eine Beispielzeile =="
cut -f1,2 "$TMP/findings" | sed 's#^[0-9]*:[0-9]*: error: ##' \
    | sed "s/'[^']*'/'X'/" | sort -u -t"$(printf '\t')" -k1,1 \
    | while IFS=$(printf '\t') read -r m c; do printf '  %-34s | %s\n' "$m" "$c"; done
