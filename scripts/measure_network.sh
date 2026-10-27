#!/bin/bash
# Misst das Verhalten der DirectPlay-Befehle im originalen Blitz3D
# (Arbeitspaket 1 in NETWORK.md). Die Messprogramme liegen in
# tests/network/original/, Programme und Protokolle landen in build/netmess/.
#
# Verwendung:  bash scripts/measure_network.sh [pfad/zu/Blitz3D] [szenario ...]
# Vorgabe fuer den Pfad: $BLITZ3D_HOME, sonst F:/dev/Blitz3D
# Szenarien: a b1 b2 c d e (ohne Angabe alle, etwa 5 Minuten)
#
# Voraussetzung: Das Windows-Feature DirectPlay (Legacykomponenten) ist
# eingeschaltet. Alles laeuft ueber 127.0.0.1 auf diesem Rechner.
#
# ACHTUNG: Die Programme oeffnen Fenster und nehmen den Fokus. Waehrend des
# Laufs nichts eintippen oder anklicken.
#
# Jedes Programm hat ein Zeitlimit. Wer es ueberschreitet (Absturzdialog,
# DirectPlay-Dialog, Haenger), wird beendet; der Fenstertitel steht dann im
# Ergebnis. Das Ergebnis steht in build/netmess/ergebnis.txt.

set -u
B3D="${1:-${BLITZ3D_HOME:-F:/dev/Blitz3D}}"
[ $# -gt 0 ] && shift
SCEN="${*:-a b1 b2 c d e}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT/tests/network/original"
OUT="$ROOT/build/netmess"
ORIG="$B3D/bin/blitzcc.exe"

if [ ! -f "$ORIG" ]; then
    echo "Kein Original gefunden unter $ORIG." >&2
    exit 2
fi
mkdir -p "$OUT"
export BLITZPATH="$(cygpath -w "$B3D")"

echo "Kompiliere mit $ORIG ..."
for f in a_host a_join b_host b_join c_host c_join d_host d_join e_probe; do
    ( cd "$SRC" && "$ORIG" -o "$(cygpath -w "$OUT")/$f.exe" "$f.bb" ) > "$OUT/$f.compile.txt" 2>&1 \
        || { echo "Fehler beim Kompilieren von $f:" >&2; cat "$OUT/$f.compile.txt" >&2; exit 1; }
done

PIDS=()
NAMES=()

# start <log> <exe> [argument]: startet ein Messprogramm im Hintergrund
start() {
    local log="$1" exe="$2"
    shift 2
    rm -f "$OUT/$log.txt"
    "$OUT/$exe.exe" "$(cygpath -w "$OUT/$log.txt")" "$@" &
    PIDS+=($!)
    NAMES+=("$log")
}

winpid() { cat "/proc/$1/winpid" 2>/dev/null; }

# stop_one <index> <grund>: beendet ein Programm hart und vermerkt den Fenstertitel
stop_one() {
    local pid="${PIDS[$1]}" log="${NAMES[$1]}" w title
    kill -0 "$pid" 2>/dev/null || return
    w=$(winpid "$pid")
    title=$(tasklist //v //fo csv //nh //fi "PID eq $w" 2>/dev/null | tr -d '\r' | awk -F'","' '{print $NF}' | tr -d '"')
    echo "$2, Fenstertitel: [$title]" >> "$OUT/$log.txt"
    taskkill //F //PID "$w" > /dev/null 2>&1
    wait "$pid" 2>/dev/null
}

# stop_name <log> <grund>
stop_name() {
    local i
    for i in "${!NAMES[@]}"; do
        [ "${NAMES[$i]}" = "$1" ] && stop_one "$i" "$2"
    done
}

# wait_all <sekunden>: wartet auf alle laufenden Programme, beendet Nachzuegler
wait_all() {
    local end=$((SECONDS + $1)) i alive
    while [ $SECONDS -lt $end ]; do
        alive=0
        for i in "${!PIDS[@]}"; do kill -0 "${PIDS[$i]}" 2>/dev/null && alive=1; done
        [ $alive = 0 ] && break
        sleep 0.2
    done
    for i in "${!PIDS[@]}"; do stop_one "$i" "ABBRUCH: Zeitlimit $1 s ueberschritten"; done
    PIDS=()
    NAMES=()
}

for s in $SCEN; do
    case $s in
    a)  echo "Szenario A: Ereignisse und Nachrichten (etwa 45 s)"
        start a_host a_host
        sleep 1.5; start a_j1 a_join J1
        sleep 8.5; start a_j2 a_join J2
        wait_all 50 ;;
    b1) echo "Szenario B1: Host verlaesst das Spiel (etwa 40 s)"
        start b1_host b_host stop
        sleep 1.5; start b1_j1 b_join J1
        sleep 1.5; start b1_j2 b_join J2
        wait_all 45 ;;
    b2) echo "Szenario B2: Host-Prozess wird beendet (etwa 40 s)"
        start b2_host b_host kill
        sleep 1.5; start b2_j1 b_join J1
        sleep 1.5; start b2_j2 b_join J2
        sleep 15
        stop_name b2_host "ABBRUCH: vom Skript beendet (Szenario B2)"
        wait_all 35 ;;
    c)  echo "Szenario C: Beitrittsvarianten (bis 100 s)"
        start c_host c_host 120000
        HOSTI=${#PIDS[@]}
        sleep 1.5
        n=0
        for v in "mess|127.0.0.1" "falsch|127.0.0.1" "|127.0.0.1" "mess|localhost" "mess|0.0.0.0" "mess|"; do
            n=$((n + 1))
            start c_join$n c_join "$v"
            JI=$((${#PIDS[@]} - 1))
            end=$((SECONDS + 15))
            while [ $SECONDS -lt $end ] && kill -0 "${PIDS[$JI]}" 2>/dev/null; do sleep 0.2; done
            stop_one "$JI" "ABBRUCH: Zeitlimit 15 s ueberschritten"
        done
        stop_name c_host "Ende Szenario C: vom Skript beendet"
        wait_all 5 ;;
    d)  echo "Szenario D: Typen, Groessen, viele Nachrichten (etwa 30 s)"
        start d_host d_host
        sleep 1.5; start d_join d_join
        wait_all 45 ;;
    e)  echo "Szenario E: Fehlerfaelle (bis 90 s)"
        for c in recv_ohne_spiel send_ohne_spiel player_ohne_spiel send_ohne_spieler \
                 send_fremde_id name_unbekannt delete_unbekannt msg_ohne_recv host_zweimal; do
            start "e_$c" e_probe "$c"
            wait_all 10
        done ;;
    *)  echo "Unbekanntes Szenario $s" >&2 ;;
    esac
done

R="$OUT/ergebnis.txt"
{
    echo "DirectPlay-Messung am Original, $(date '+%Y-%m-%d %H:%M'), Szenarien: $SCEN"
    for f in "$OUT"/a_*.txt "$OUT"/b1_*.txt "$OUT"/b2_*.txt "$OUT"/c_*.txt "$OUT"/d_*.txt "$OUT"/e_*.txt; do
        [ -f "$f" ] || continue
        case "$f" in *.compile.txt) continue ;; esac
        echo
        echo "=== $(basename "$f" .txt)"
        tr -d '\r' < "$f"
    done
} > "$R"
echo "Ergebnis: $R"
