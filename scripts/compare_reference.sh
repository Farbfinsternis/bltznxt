#!/bin/bash
# Vergleicht unseren Compiler mit dem originalen Blitz3D (blitzcc 1.108c),
# falls es lokal installiert ist. Geprueft wird nur das Frontend: nimmt der
# jeweilige Compiler ein Programm an, und wenn nicht, mit welcher Meldung.
# Beide laufen mit -c, es wird also nichts ausgefuehrt.
#
# Verwendung:  bash scripts/compare_reference.sh [pfad/zu/Blitz3D]
# Vorgabe fuer den Pfad: $BLITZ3D_HOME, sonst G:/dev/Blitz3D
#
# Warum das hier steht: Semantikfragen werden in diesem Projekt gegen die
# quelloffene Referenz belegt statt geraten. Der Quelltext sagt, was gemeint
# ist; dieses Skript sagt, was tatsaechlich passiert. Am 2026-09-07 hat es
# binnen Minuten zwei falsche Annahmen in frisch committeten Tests gefunden,
# die aus dem Quelltext plausibel abgeleitet worden waren.
#
# Grenzen: Ein Vergleich der Programmausgabe ist damit NICHT moeglich - ein
# Blitz3D-Programm schreibt in sein eigenes Fenster, nicht auf stdout. Und
# Befehle, die es im Original nicht gibt (unsere SDL-Schicht), meldet es als
# "Function 'x' not found"; das ist eine Bibliotheks-, keine Sprachdifferenz
# und wird unten getrennt gezaehlt.

set -u
B3D="${1:-${BLITZ3D_HOME:-G:/dev/Blitz3D}}"
ORIG="$B3D/bin/blitzcc.exe"
OURS="bin/blitzcc.exe"

if [ ! -f "$ORIG" ]; then
    echo "Kein Original gefunden unter $ORIG - uebersprungen."
    echo "Pfad per Argument oder \$BLITZ3D_HOME angeben."
    exit 0
fi
if [ ! -f "$OURS" ]; then
    echo "$OURS fehlt - erst 'mingw32-make -C build' laufen lassen." >&2
    exit 2
fi

export BLITZPATH="$B3D"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

same=0; lib=0; n=0
declare -a WE_ACCEPT_IT_REJECTS=()
declare -a WE_REJECT_IT_ACCEPTS=()

for f in $(ls tests/*.bb) $(find examples -name "*.bb"); do
    n=$((n+1))
    b=$(echo "$f" | sed 's#[/\]#_#g')
    cp "$f" "$TMP/$b"

    "$OURS" -c "$TMP/$b" >/dev/null 2>&1 && we=ok || we=no
    "$ORIG" -c "$TMP/$b" > "$TMP/$b.orig" 2>&1 && it=ok || it=no
    msg=$(grep '^"' "$TMP/$b.orig" | head -1 | sed 's#.*\.bb"##')

    if [ "$we" = "$it" ]; then same=$((same+1)); continue; fi

    if [ "$we" = ok ]; then
        case "$msg" in
            *"not found"*) lib=$((lib+1));;                  # Bibliotheksdifferenz
            *) WE_ACCEPT_IT_REJECTS+=("$f$msg");;
        esac
    else
        WE_REJECT_IT_ACCEPTS+=("$f")
    fi
done

echo "Geprueft: $n Programme gegen $ORIG"
echo "Gleiches Urteil: $same    Bibliotheksdifferenz (Befehl im Original unbekannt): $lib"
echo
echo "Wir nehmen an, das Original lehnt ab  (${#WE_ACCEPT_IT_REJECTS[@]}):"
if [ ${#WE_ACCEPT_IT_REJECTS[@]} -eq 0 ]; then echo "  (keine)"; else
    for e in "${WE_ACCEPT_IT_REJECTS[@]}"; do echo "  $e"; done
fi
echo
echo "Wir lehnen ab, das Original nimmt an  (${#WE_REJECT_IT_ACCEPTS[@]}):"
if [ ${#WE_REJECT_IT_ACCEPTS[@]} -eq 0 ]; then echo "  (keine)"; else
    for e in "${WE_REJECT_IT_ACCEPTS[@]}"; do echo "  $e"; done
fi
echo
echo "Beide Listen sind Fundlisten, keine Fehlerliste: ein neg_*.bb, das eine"
echo "bewusste Zusatzdiagnose prueft, steht zu Recht in der zweiten."
