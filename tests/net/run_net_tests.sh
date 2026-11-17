#!/bin/bash
# Mehrprozess-Tests der Net-Befehle (bb_net.h). Jeder Unterordner von
# tests/net/ ist ein Fall: mehrere Programme, die gleichzeitig laufen und
# sich ueber 127.0.0.1 unterhalten. Die Datei case.txt sagt, wann was startet:
#
#   start <prog> <ms> [NAME=WERT ...]   Programm <prog>.bb nach <ms> starten,
#                                       mit zusaetzlichen Umgebungsvariablen
#   kill <prog> <ms>                    Programm nach <ms> hart beenden
#   server <ms> <port>                  Referenz-Listen-Server (listserver/
#                                       listserver.js, Node.js) starten; ohne
#                                       Node.js wird der Fall uebersprungen
#   igd <ms> <port> [opt ...]           nachgebauter Router fuer UPnP
#                                       (fake_igd.js, Node.js), Protokoll der
#                                       Aufrufe auf TCP <port> + 1
#   timeout <s>                         Zeitlimit des Falls (Vorgabe 40)
#   v4only                              Fall nur im IPv4-Durchgang (UPnP leitet
#                                       nur IPv4 weiter)
#
# Verglichen wird die Ausgabe jedes Programms mit <prog>.expected (ohne
# Wagenruecklauf) und, falls vorhanden, stderr mit <prog>.expected_stderr.
# Alle Sockets binden an eine Adresse dieses Rechners (BLTZNXT_NET_BIND): das
# bleibt auf dem Rechner und ruft keine Firewall-Abfrage hervor.
#
# Jeder Fall laeuft zweimal: ueber 127.0.0.1 (IPv4) und ueber ::1 (IPv6). Im
# IPv6-Durchgang nennt NET_TEST_LO den Programmen die Adresse (common.bb, Lo
# und LoUrl), und in case.txt und den Erwartungen wird aus 127.0.0.1:port
# [::1]:port und aus 127.0.0.1 ::1. NET_FAMILIES=4 laesst nur IPv4 laufen.
#
# Verwendung:  bash tests/net/run_net_tests.sh [fall ...]   (aus Projektroot)

export BLTZNXT_WINDOWED=1
FAMILIES="${NET_FAMILIES:-4 6}"
PASS=0; FAIL=0; SKIP=0
CASES="${*:-$(cd tests/net && ls -d */ | tr -d /)}"
NODE=""
command -v node > /dev/null 2>&1 && NODE=node

# Text auf stdin fuer den IPv6-Durchgang umschreiben
v6text() { sed -e 's/127\.0\.0\.1:\([0-9]\)/[::1]:\1/g' -e 's/127\.0\.0\.1/::1/g'; }

for c in $CASES; do
    dir="tests/net/$c"
    [ -f "$dir/case.txt" ] || continue
    if grep -q "^server\|^igd" "$dir/case.txt" && [ -z "$NODE" ]; then
        echo "SKIP (no Node.js for the list server or router): net/$c"
        ((SKIP++)); continue
    fi
    out="bin/net_$c"
    mkdir -p "$out"
    rm -f "$out"/*.out "$out"/*.err "$out"/*.log
    ok=1
    # parallel kompilieren: g++ braucht je Programm etwa 10 s
    for f in "$dir"/*.bb; do
        p=$(basename "$f" .bb)
        ( bin/blitzcc.exe "$f" -o "$out/net_${c}_$p" -q 2> "$out/$p.compile" || echo fail > "$out/$p.cfail" ) &
    done
    wait
    for f in "$dir"/*.bb; do
        p=$(basename "$f" .bb)
        if [ -f "$out/$p.cfail" ]; then
            echo "FAIL (compile error): net/$c/$p.bb"; head -3 "$out/$p.compile"; ok=0
            rm -f "$out/$p.cfail"
        fi
    done
    [ $ok = 1 ] || { ((FAIL++)); continue; }

    for fam in $FAMILIES; do
        if [ "$fam" = 6 ]; then
            grep -q "^v4only" "$dir/case.txt" && continue
            export BLTZNXT_NET_BIND=::1 NET_TEST_LO=::1
            lohost=::1; tag=" (IPv6)"; conv=v6text
        else
            export BLTZNXT_NET_BIND=127.0.0.1
            unset NET_TEST_LO
            lohost=127.0.0.1; tag=""; conv=cat
        fi
        rm -f "$out"/*.out "$out"/*.err "$out"/*.log
        ok=1
        limit=40
        pids=()
        while read -r cmd a b rest; do
            case "$cmd" in
            start)
                ( sleep "$(awk "BEGIN{print $b/1000}")"
                  env $rest "$out/net_${c}_$a.exe" > "$out/$a.out" 2> "$out/$a.err" ) &
                pids+=($!) ;;
            kill)
                ( sleep "$(awk "BEGIN{print $b/1000}")"
                  taskkill //F //IM "net_${c}_$a.exe" > /dev/null 2>&1 ) &
                pids+=($!) ;;
            server)
                ( sleep "$(awk "BEGIN{print $a/1000}")"
                  exec $NODE listserver/listserver.js --host "$lohost" --port "$b" --lifetime "$limit" > "$out/server.log" 2>&1 ) &
                SERVERPIDS="$SERVERPIDS $!" ;;
            igd)
                ( sleep "$(awk "BEGIN{print $a/1000}")"
                  exec $NODE tests/net/fake_igd.js --port "$b" --lifetime "$limit" $rest > "$out/igd.log" 2>&1 ) &
                SERVERPIDS="$SERVERPIDS $!" ;;
            timeout) limit="$a" ;;
            esac
        done < <(tr -d '\r' < "$dir/case.txt" | $conv)

        end=$((SECONDS + limit))
        while [ $SECONDS -lt $end ]; do
            alive=0
            for j in $(jobs -rp); do
                [[ " $SERVERPIDS " == *" $j "* ]] || alive=1
            done
            [ $alive = 0 ] && break
            sleep 0.2
        done
        timedout=0
        for f in "$dir"/*.bb; do
            p=$(basename "$f" .bb)
            if tasklist //fi "IMAGENAME eq net_${c}_$p.exe" 2>/dev/null | grep -qi "net_${c}_$p"; then
                timedout=1
                taskkill //F //IM "net_${c}_$p.exe" > /dev/null 2>&1
            fi
        done
        [ -n "$SERVERPIDS" ] && kill $SERVERPIDS 2> /dev/null
        unset SERVERPIDS
        wait 2>/dev/null

        for f in "$dir"/*.expected; do
            p=$(basename "$f" .expected)
            actual=$(tr -d '\r' < "$out/$p.out" 2>/dev/null)
            expected=$(tr -d '\r' < "$f" | $conv)
            if [ "$actual" != "$expected" ]; then
                ok=0
                echo "FAIL (output mismatch): net/$c/$p$tag"
                diff <(echo "$expected") <(echo "$actual") | head -8 | sed 's/^/    /'
            fi
            if [ -f "$dir/$p.expected_stderr" ]; then
                actual_err=$(grep -v '^\[' "$out/$p.err" 2>/dev/null | tr -d '\r')
                expected_err=$(tr -d '\r' < "$dir/$p.expected_stderr" | $conv)
                if [ "$actual_err" != "$expected_err" ]; then
                    ok=0
                    echo "FAIL (stderr mismatch): net/$c/$p$tag"
                    diff <(echo "$expected_err") <(echo "$actual_err") | head -8 | sed 's/^/    /'
                fi
            fi
        done
        [ $timedout = 1 ] && { ok=0; echo "FAIL (time limit ${limit} s): net/$c$tag"; }
        if [ $ok = 1 ]; then echo "PASS: net/$c$tag"; ((PASS++)); else ((FAIL++)); fi
    done
done

echo ""
echo "Result: $PASS passed, $FAIL failed, $SKIP skipped"
[ $FAIL -eq 0 ]
