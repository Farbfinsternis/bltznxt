# BlitzNext — Refactoring-Plan

Dieses Dokument beschreibt technische Schulden und geplante Code-Qualitäts-
verbesserungen. Es ist bewusst von der `roadmap.md` getrennt — dort werden
neue Sprachfeatures (Milestones) geplant, hier geht es um interne Qualität.

Priorität: **R1 → R2 → R3 → R4 → R5** (sequential, je ~1 Session)
R6 + R7 sind zurückgestellt (hohes Risiko, geringer kurzfristiger Nutzen).

Status-Symbole: ☐ offen · ✓ erledigt · ⏸ zurückgestellt

---

## R1 — Undefined Behavior & C-Style Casts ✓
*Dateien: `emitter.h`, `preprocessor.h`, `bb_math.h`, `parser.h`*
*Risiko: sehr gering — kein Verhaltensunterschied für ASCII-Input*

### R1.1 — `::tolower` / `::toupper` ohne `unsigned char`-Cast (UB)

`::toupper(char)` und `::tolower(char)` sind auf Plattformen mit `signed char`
Undefined Behavior für Werte > 127. Alle 9 Fundstellen ersetzen:

```cpp
// Vorher (UB):
std::transform(s.begin(), s.end(), s.begin(), ::tolower);

// Nachher (korrekt):
std::transform(s.begin(), s.end(), s.begin(),
               [](unsigned char c){ return (char)std::tolower(c); });
```

Fundstellen:
- `preprocessor.h:48` — `::toupper` (1×)
- `emitter.h` — `::tolower` (8×, Zeilen 31, 128, 138, 346, 464, 571, 838, 859)

Hinweis: `lexer.h` (toUpper), `bb_string.h` (bb_Upper/bb_Lower) sind bereits
korrekt — diese dienten als Vorlage.

### R1.2 — C-Style Casts in `bb_math.h`

```cpp
// Vorher:
__bb_rnd_seed__ = (unsigned int)seed;
return (int)__bb_rnd_seed__;

// Nachher:
__bb_rnd_seed__ = static_cast<unsigned int>(seed);
return static_cast<int>(__bb_rnd_seed__);
```

### R1.3 — `int savedPos` → `size_t` in `parser.h`

`pos` ist `size_t`; der temporäre Save sollte denselben Typ haben.

```cpp
// Vorher (Zeile 111):
int savedPos = pos;

// Nachher:
size_t savedPos = pos;
```

**Verifikation**: Rebuild + alle `tests/test_*.bb` müssen weiter passen.

> **Ergebnis**: ✓ 13/14 Tests PASS. `test_m16_iteration.bb` schlägt mit einem
> *pre-existing* Parser-Bug fehl: `(First Node)\val` — Feldzugriff auf einen
> geklammerten Ausdruck wird nicht korrekt emittiert. Kein Zusammenhang mit R1.

---

## R2 — Runtime-Konsistenz & stille Fehler ✓
*Dateien: `bb_runtime.h`, `bb_string.h`, `emitter.h`*
*Risiko: gering — nur interne Ausgaben und Fallback-Verhalten*

### R2.1 — `bb_DataVal::operator bbString()` für KIND_FLOAT

`std::to_string(float)` erzeugt trailing zeros ("3.140000").
Konsistent mit `bb_Str(double)` → `%g` verwenden.

```cpp
// Vorher:
case KIND_FLOAT: return std::to_string(fval);

// Nachher:
case KIND_FLOAT: {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%g", fval);
    return buf;
}
```

### R2.2 — `bb_DataRead()` out-of-data → stderr-Warnung

Blitz3D gibt beim Lesen über das Ende der Data-Liste hinaus einen Laufzeitfehler.
Aktuell: stilles `return bb_DataVal(0)`.

```cpp
// Nachher:
if (__bb_data_idx__ >= __bb_data_pool__.size()) {
    std::cerr << "[runtime] Read: past end of Data\n";
    return bb_DataVal(0);
}
return __bb_data_pool__[__bb_data_idx__++];
```

### R2.3 — Delete-Fallback → Compiler-Warning auf stderr

Wenn `getExprTypeName()` leer zurückgibt, wird aktuell stillschweigend
ungültiger Code generiert. Mindestens eine Warnung ausgeben:

```cpp
} else {
    std::cerr << "[warning] Delete: type indeterminate at compile time"
              << " (line " << node->line << ")\n";
    output << ind();
    emitExpr(node->object.get());
    output << " = nullptr; // Delete (type unknown)\n";
}
```

### R2.4 — `bb_Int(string)` / `bb_Float(string)` exception-safe machen

`std::stoi` / `std::stof` werfen bei ungültigem Input eine Exception.
Der generierte Code hat kein try/catch → unkontrollierter Abbruch.

```cpp
// Nachher in bb_string.h:
inline int bb_Int(const bbString &s) {
    try { return std::stoi(s); }
    catch (...) {
        std::cerr << "[runtime] Int(): invalid value \"" << s << "\"\n";
        return 0;
    }
}
inline float bb_Float(const bbString &s) {
    try { return std::stof(s); }
    catch (...) {
        std::cerr << "[runtime] Float(): invalid value \"" << s << "\"\n";
        return 0.0f;
    }
}
```

**Verifikation**: Rebuild + alle Tests + neuen Test `test_runtime_errors.bb`
mit ungültigem Int/Float-Input prüfen, dass kein Crash auftritt.

> **Ergebnis**: ✓ 13/14 Tests PASS + `test_runtime_errors.bb` PASS.
> `test_m16_iteration.bb` schlägt weiterhin mit dem bekannten pre-existing Bug fehl
> (kein Zusammenhang mit R2). `bb_Int("abc")` → 0 + stderr-Warnung, kein Crash. ✓

---

## R3 — Reserved Identifier Names ✓
*Dateien: `bb_math.h`, `bb_runtime.h`, `emitter.h`*
*Risiko: gering — rein mechanischer Rename, kein Logikwechsel*

Namen mit führendem `__` sind im C++-Standard global für die Implementierung
reserviert. Alle ersetzen durch Varianten mit einfachem trailing `_`.

| Alter Name | Neuer Name | Dateien |
|------------|------------|---------|
| `__bb_rnd_seed__` | `bb_rnd_seed_` | `bb_math.h` |
| `__bb_data_pool__` | `bb_data_pool_` | `bb_runtime.h`, `emitter.h` (collectData) |
| `__bb_data_idx__` | `bb_data_idx_` | `bb_runtime.h` |
| `__bb_TypeName_head__` | `bb_TypeName_head_` | `emitter.h` (generiert, 4 Stellen) |
| `__bb_TypeName_tail__` | `bb_TypeName_tail_` | `emitter.h` (generiert, 4 Stellen) |
| `_fe_cur_varName_` | `bb_fe_varName_` | `emitter.h` (ForEach-Loop, 2 Stellen) |

Betroffene Emitter-Stellen für Type-Namen:
- `emitTypeDecl()` — Struct-Emission (head/tail-Deklaration, _New, _Delete, _Unlink, _InsertBefore, _InsertAfter)
- `visit(FirstExpr)`, `visit(LastExpr)` — Lesezugriffe
- `visit(ForEachStmt)` — Loop-Cursor-Variable

**Verifikation**: Rebuild + alle Tests. Die generierten `.exe`-Dateien müssen
identisches Verhalten zeigen (nur interne Symbolnamen ändern sich).

> **Ergebnis**: ✓ 14/15 Tests PASS. `test_m16_iteration.bb` schlägt weiterhin
> mit dem bekannten pre-existing Bug fehl. Alle Symbolnamen korrekt umbenannt;
> `ast.h`-Kommentare ebenfalls aktualisiert. ✓

---

## R4 — DRY: Type-Hint-Helper im Emitter ✓
*Dateien: `emitter.h`*
*Risiko: mittel — 6 Callsites; sorgfältig testen*

Die Logik `typeHint → (cppType, defaultValue)` ist in 6 Methoden dupliziert.
Eine private statische Hilfsmethode konsolidiert das auf eine Quelle.

### Neue Methode

```cpp
// Returns {cppType, defaultValue} for a Blitz3D type hint.
// Examples: "$" → {"bbString", "\"\""}, ".Vec" → {"bb_Vec *", "nullptr"}
// Empty hint or "%" → {"int", "0"} (Blitz3D numeric default).
static std::pair<std::string, std::string>
hintToType(const std::string &hint) {
    if (hint == "$")
        return {"bbString", "\"\""};
    if (hint == "#" || hint == "!")
        return {"float", "0.0f"};
    if (!hint.empty() && hint[0] == '.') {
        return {"bb_" + hint.substr(1) + " *", "nullptr"};
    }
    return {"int", "0"}; // "%" or empty → int
}
```

### Ablösungsstellen

| Methode | Ersetzen durch |
|---------|---------------|
| `visit(VarDecl)` LOCAL-Pfad | `auto [type, defVal] = hintToType(node->typeHint);` |
| `collectGlobals()` | idem |
| `visit(FunctionDecl)` params | `auto [ptype, std::ignore] = hintToType(hint);` |
| `visit(DimStmt)` | `auto [elemType, std::ignore] = hintToType(node->typeHint);` |
| `visit(ReadStmt)` | `auto [type, std::ignore] = hintToType(node->typeHint);` |
| `emitTypeDecl()` Felder | `auto [ftype, fdef] = hintToType(f.typeHint);` |

Hinweis: `visit(ConstDecl)` bleibt unverändert — Konstanten verwenden
`constexpr`/`const` statt `auto` und haben keine Defaultwerte.

**Verifikation**: Rebuild + alle Tests + neuen Test mit allen Typkombinationen
(`Local x%, y#, z$, w.Type`).

> **Ergebnis**: ✓ 14/15 Tests PASS. `test_m16_iteration.bb` weiterhin pre-existing Bug.
> Alle 6 Callsites konsolidiert; `hintToType()` als private static method neben `mapOp()`. ✓

---

## R5 — Test-Infrastruktur ✓
*Dateien: `tests/run_tests.sh` (neu), `tests/neg_*.bb` (neu)*
*Risiko: sehr gering — nur additive Änderungen*

### R5.1 — Test-Runner-Script

`tests/run_tests.sh`:
```bash
#!/bin/bash
# Kompiliert alle test_*.bb; Exit-Code 0 = PASS, sonst FAIL.
# Verwendung: bash tests/run_tests.sh
PASS=0; FAIL=0

for f in tests/test_*.bb; do
    name=$(basename "$f" .bb)
    if bin/blitzcc.exe "$f" -o bin/"$name" -q 2>/dev/null; then
        # Optionaler Output-Vergleich wenn .expected existiert
        if [ -f "tests/${name}.expected" ]; then
            actual=$(bin/"${name}".exe 2>/dev/null)
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

echo ""
echo "Result: $PASS passed, $FAIL failed"
[ $FAIL -eq 0 ]
```

### R5.2 — Negativtests (`tests/neg_*.bb`)

Programme, bei denen `blitzcc` Exit-Code 1 zurückgeben *muss*:
- `neg_unclosed_if.bb` — `If x = 1` ohne EndIf
- `neg_missing_next.bb` — `For i = 1 To 10` ohne Next
- `neg_missing_wend.bb` — `While 1` ohne Wend

Der Runner kann Negativtests gesondert handhaben:
```bash
for f in tests/neg_*.bb; do
    if ! bin/blitzcc.exe "$f" -o bin/tmp_neg -q 2>/dev/null; then
        echo "PASS (expected error): $f"
    else
        echo "FAIL (should have errored): $f"
    fi
done
```

### R5.3 — `.expected`-Dateien für bestehende Tests

Für jeden bestehenden `test_*.bb` eine `test_*.expected`-Datei anlegen
(manuell einmal erzeugen, danach automatisch geprüft).

**Verifikation**: `bash tests/run_tests.sh` muss mit Exit-Code 0 enden.

> **Ergebnis**: ✓ 17/18 Tests PASS (14 positive + 3 Negativtests), 1 SKIP.
> `test_m16_iteration.bb` weiterhin in KNOWN_FAIL-Liste; kein Zusammenhang mit R5.
> Runner-Fix: Executables werden mit `< /dev/null` gestartet (löst `WaitKey`-Blockierung in `test_fixes.bb`).
> `.expected`-Dateien für alle 14 positiven Tests angelegt. ✓

---

## R6 — `inExprCtx`-Flag → expliziter Parameter ⏸
*Dateien: `emitter.h`*
*Risiko: mittel-hoch — alle 32 visit()-Methoden betroffen*

Das `inExprCtx`-Flag wird in 12+ Stellen mit `bool prev = ...; ... = prev;`
umgeben. Alternative: `emitExpr(node)` / `emitStmt(node)` Hilfsmethoden, die
den Kontext intern setzen.

**Zurückgestellt bis**: Ein zweiter Visitor-Typ (z.B. Type-Checker) hinzukommt.
Solange nur ein Emitter existiert, ist der Nutzen zu gering für das Risiko.

---

## R7 — `Program`-Node als Container ersetzen ⏸
*Dateien: `ast.h`, `parser.h`, `emitter.h`*
*Risiko: hoch — betrifft alle Traversallogiken*

`parseVarDecl()`, `parseDim()`, `parseConst()` geben einen `Program`-Node als
transparenten Listen-Container zurück. Das bricht das semantische Modell und
erzwingt überall einen `dynamic_cast<Program*>`-Sonderfall.

Alternative: Neue Node-Klasse `DeclGroup : ASTNode` als reiner Container.

**Zurückgestellt bis**: Die Architektur grundlegend erweitert wird (Type-Checker,
weiterer Visitor, Namensauflösungs-Pass). Aktuell zu hohem Risiko für zu
geringen Gewinn.

---

## Empfohlene Reihenfolge

```
R1 (UB)  →  R2 (Runtime)  →  R3 (Namen)  →  R4 (DRY)  →  R5 (Tests)
 ~1h          ~1h              ~1h             ~2h           ~1h

R6, R7: zurückgestellt
```

Jeder Schritt endet mit `Rebuild + alle Tests grün`.
