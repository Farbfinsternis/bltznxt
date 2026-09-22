#!/bin/bash
# BlitzNext Test Runner
# Kompiliert alle test_*.bb und neg_*.bb; Exit-Code 0 = alle erwartet PASS.
# Verwendung: bash tests/run_tests.sh   (aus Projektroot ausführen)
#
# Eine .expected-Datei muss CRLF-Zeilenenden haben: die Programme geben CRLF aus,
# und verglichen wird Zeichen fuer Zeichen. Eine mit LF geschriebene Datei faellt
# durch, obwohl das Protokoll erwartet und tatsaechlich gleich anzeigt.

PASS=0; FAIL=0; SKIP=0

# Tests mit bekannten pre-existing Bugs (nicht Refactor-Scope):
KNOWN_FAIL=""

# ---- Positive Tests ----
for f in tests/test_*.bb; do
    name=$(basename "$f" .bb)

    # Überspringe bekannte pre-existing Failures
    if [[ "$KNOWN_FAIL" == *"$name"* ]]; then
        echo "SKIP (known pre-existing bug): $f"
        ((SKIP++))
        continue
    fi

    if bin/blitzcc.exe "$f" -o bin/"$name" -q 2>/dev/null; then
        # Optionaler Output-Vergleich wenn .expected existiert
        if [ -f "tests/${name}.expected" ]; then
            actual=$(bin/"${name}".exe </dev/null 2>/dev/null)
            expected=$(cat "tests/${name}.expected")
            if [ "$actual" = "$expected" ]; then
                echo "PASS: $f"
                ((PASS++))
            else
                echo "FAIL (output mismatch): $f"
                echo "  expected: $(echo "$expected" | head -3)"
                echo "  actual:   $(echo "$actual"   | head -3)"
                ((FAIL++))
            fi
        else
            echo "PASS (compile only): $f"
            ((PASS++))
        fi
    else
        echo "FAIL (compile error): $f"
        ((FAIL++))
    fi
done

# ---- Negativtests ----
# Ein Negativtest muss fehlschlagen. Liegt zusaetzlich eine .expected_err vor,
# muss die Diagnose auch woertlich stimmen - Datei, Zeile, Spalte, Text. Das
# ist die einzige Art, ein Zeilen-Mapping (WEAK-13) ueberhaupt zu pruefen.
for f in tests/neg_*.bb; do
    [ -f "$f" ] || continue
    name=$(basename "$f" .bb)
    actual_err=$(bin/blitzcc.exe "$f" -o bin/tmp_neg -q 2>&1 >/dev/null)
    if [ $? -eq 0 ]; then
        echo "FAIL (should have errored): $f"
        ((FAIL++))
        continue
    fi
    if [ -f "tests/${name}.expected_err" ]; then
        expected_err=$(cat "tests/${name}.expected_err")
        if [ "$actual_err" = "$expected_err" ]; then
            echo "PASS (expected error + message): $f"
            ((PASS++))
        else
            echo "FAIL (diagnostic mismatch): $f"
            echo "  expected: $expected_err"
            echo "  actual:   $actual_err"
            ((FAIL++))
        fi
    else
        echo "PASS (expected error): $f"
        ((PASS++))
    fi
done

echo ""
echo "Result: $PASS passed, $FAIL failed, $SKIP skipped"
[ $FAIL -eq 0 ]
