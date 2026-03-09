# BLTZNXT — Bug List & Analyse

Vollständige Analyse des Compiler-Quellcodes. Behobene Einträge sind mit ✅ markiert.

---

## 🔴 Bugs (Fehler)

### ✅ BUG-01: `SHR` und `SAR` erzeugen identischen C++-Code
- **Datei:** [emitter.h](file:///g:/dev/projects/bltznxt/src/compiler/emitter.h#L899-L901)
- **Zeile:** 900–901
- **Problem:** Beide Operatoren werden auf `>>` gemappt. In Blitz3D ist `SHR` ein *unsigned* shift (logisch), `SAR` ein *signed* shift (arithmetisch). In C++ ist `>>` auf signed int implementation-defined, aber auf unsigned int logisch. Der Emitter müsste `SHR` als Cast auf `unsigned` + `>>` emittieren, um korrektes Verhalten zu garantieren.
- **Auswirkung:** Negativzahlen mit `SHR` liefern falsches Ergebnis.
- **Behoben:** `SHR` als Sonderfall in `visit(BinaryExpr*)` behandelt — emittiert `(int)((unsigned int)(lhs) >> rhs)`. `SAR` bleibt korrekt als signed `>>`.

### ✅ BUG-02: Version-Mismatch in `blitzcc.cpp`
- **Datei:** [blitzcc.cpp](file:///g:/dev/projects/bltznxt/src/compiler/blitzcc.cpp#L402-L434)
- **Problem:** `-h` zeigt `v0.1.4`, `-v` zeigt `v0.2.7`. Zwei verschiedene Versionsstrings an zwei Stellen.
- **Auswirkung:** Verwirrung bei Nutzern über die tatsächliche Version.
- **Behoben:** Beide Strings auf `v0.3.3` vereinheitlicht.

### ✅ BUG-03: `Rand(max)` bei `max ≤ 0` → Division durch Null / UB
- **Datei:** [bb_math.h](file:///g:/dev/projects/bltznxt/src/compiler/bb_math.h#L66)
- **Zeile:** 66
- **Problem:** `bb_Rand(int max)` rechnet `1 + (std::rand() % max)`. Bei `max == 0` → undefiniertes Verhalten (Division durch Null). Bei negativem `max` → unerwartetes Resultat.
- **Auswirkung:** Crash / UB bei falschem Nutzerinput.
- **Behoben:** Guard `if (max < 1) max = 1;` vor dem Modulo.

### ✅ BUG-04: `Rand(min, max)` bei `min > max` → UB
- **Datei:** [bb_math.h](file:///g:/dev/projects/bltznxt/src/compiler/bb_math.h#L67)
- **Zeile:** 67
- **Problem:** `bb_Rand(int min, int max)` rechnet `min + (std::rand() % (max - min + 1))`. Wenn `min > max`, ist `(max - min + 1)` negativ → Modulo mit negativem Divisor → UB.
- **Behoben:** `if (min > max) std::swap(min, max);` vor dem Modulo.

### ✅ BUG-05: `bb_Eof()` liefert falsches Ergebnis bei Text-Mode-Dateien (Windows)
- **Datei:** [bb_file.h](file:///g:/dev/projects/bltznxt/src/compiler/bb_file.h#L78-L86)
- **Problem:** `bb_Eof()` verwendet `fseek(SEEK_END)` + `ftell()` und vergleicht `cur >= end`. Die Dateiposition wird korrekt wiederhergestellt (kein Seiteneffekt im single-threaded Betrieb). Das eigentliche Problem: Auf Windows liefert `ftell()` im Text-Mode (`"r"`) keine arithmetisch vergleichbaren Byte-Offsets, weil CRLF-Sequenzen transparent übersetzt werden. `cur >= end` kann dadurch falsch-positiv oder falsch-negativ ausfallen. Für Binary-Mode (`"rb"`) funktioniert die Implementierung korrekt.
- **Auswirkung:** `Eof()` liefert falsches Ergebnis beim Lesen von Textdateien auf Windows.
- **Behoben:** `bb_Eof()` verwendet jetzt `fgetc`+`ungetc` (Peek) — korrekt für Binary- und Text-Mode, effizienter (1 Operation statt 2 fseeks). Gleichzeitig RED-08 behoben: gemeinsamer Helper `bb_file_remaining_()` extrahiert, den `bb_ReadAvail()` nutzt.

### ✅ BUG-06: Lexer erkennt kein Hex- oder Binliteral
- **Datei:** [lexer.h](file:///g:/dev/projects/bltznxt/src/compiler/lexer.h#L86-L102)
- **Problem:** Blitz3D unterstützt `$FF` (Hex) und `%10110` (Binärliterale). Der Lexer parst nur dezimale Zahlen und `.`-Dezimalzahlen. Hex/Bin-Literale werden als Operator `$` / `%` + ID/Zahl geparst → Parse-Fehler oder falsche Semantik.
- **Behoben:** `lexHexLiteral()` und `lexBinLiteral()` in `lexer.h` hinzugefügt. Dispatch in `tokenize()` vor `lexOperator()`. Beide Methoden fallen auf `OPERATOR` zurück wenn keine Ziffern folgen — Type-Hints (`a$`, `a%`) bleiben korrekt.

### ✅ BUG-07: String-Escape-Sequenzen nicht unterstützt
- **Datei:** [emitter.h](file:///g:/dev/projects/bltznxt/src/compiler/emitter.h#L84-L89)
- **Problem:** `LiteralExpr` Strings werden direkt als `"value"` in C++ emittiert. Wenn der Blitz3D-String Zeichen wie `\`, `"`, oder Newlines enthält, wird ungültiger C++-Code erzeugt. Es fehlt ein Escaping-Schritt.
- **Auswirkung:** Kompilierungsfehler bei Strings mit Sonderzeichen.
- **Behoben:** `escapeCppString()` als private statische Methode in `emitter.h` hinzugefügt. Behandelt `\`→`\\`, `"`→`\"`, `\r`→`\r`, `\t`→`\t`, sonstige Steuerzeichen→`\xNN`. `visit(LiteralExpr*)` nutzt sie für alle `STRING_LIT`-Token.

### ~~BUG-08: `bb_DataVal(int)` Konstruktor — Doppel-Literal Ambiguität~~ → verschoben nach WEAK-11

---

## 🟡 Schwachstellen (Weaknesses)

### ⏸ WEAK-01: Alles in Header-Dateien — keine `.cpp`-Separation *(zurückgestellt)*
- **Dateien:** Alle `bb_*.h`, `lexer.h`, `parser.h`, `emitter.h`
- **Problem:** Der gesamte Compiler und die gesamte Runtime liegen in Header-Dateien mit `inline`-Funktionen. Dies führt zu:
  - Langen Compile-Zeiten bei wachsendem Projekt
  - Code-Bloat durch Inkludierung in jede Translation Unit
  - Keine eigenständige Bibliotheksstruktur möglich
- **Auswirkung:** Schwer wartbar bei zunehmender Projektgröße.
- **Entscheidung (2026-03-08):** Vorerst ignoriert. Die Runtime-Header werden nicht in `blitzcc.cpp` eingebunden — sie werden als Text in die generierten `.cpp`-Dateien emittiert. Es gibt daher genau eine TU, der Build dauert ~3 Sekunden. Der Aufwand (×12 Dateien aufteilen, CMake-Library, inline entfernen) übersteigt den aktuellen Nutzen deutlich. Neu bewerten wenn Compile-Zeit spürbar steigt oder eine eigenständige Runtime-Library benötigt wird.

### ✅ WEAK-02: Keine Fehlerlimitierung im Parser
- **Datei:** [parser.h](file:///g:/dev/projects/bltznxt/src/compiler/parser.h#L44-L49)
- **Problem:** `error()` inkrementiert `errorCount`, aber es gibt kein Limit. Bei einer schadhaften Quelldatei können hunderte Fehler kaskadieren (Error-Avalanche). Ein Limit (z.B. 20 Fehler, dann Abbruch) wäre sinnvoll.
- **Behoben:** `kMaxErrors = 20` als `static constexpr`. `error()` setzt bei Erreichen `tooManyErrors_ = true` und gibt eine Fatal-Meldung aus. `atEnd()` prüft das Flag — der Parser bricht dadurch in allen Rekursionsebenen sauber ab.

### ✅ WEAK-03 (Stufe 1): Kein semantischer Analyse-Pass
- **Problem:** Der Compiler geht direkt von Parse → C++ Emission. Es gibt keinen Zwischenschritt für:
  - Typprüfung (type checking)
  - Undefinierte-Variable-Erkennung
  - Undefinierte-Funktion-Erkennung
  - Overload-Auflösung
- **Auswirkung:** Fehler werden erst beim g++-Kompilieren des generierten C++ sichtbar, mit kryptischen C++-Fehlermeldungen statt Blitz3D-Zeilennummern.
- **Behoben (Stufe 1):** `checkCalls()` in `blitzcc.cpp` — AST-Walk nach `parser.parse()`, vor `emitter.emit()`. Meldet alle `CallExpr`-Namen, die weder in `kCommands[]` noch als `Function`-Deklaration bekannt sind, mit Dateiname + Zeilennummer im IDE-Format. Gibt bei Fehlern `return 1` zurück.
- **Offen (Stufe 2):** "Did you mean?"-Vorschläge (Levenshtein-Distanz auf `kCommands`-Namen).
- **Offen (Stufe 3):** Arität- und Typprüfung anhand `kCommands[].sig`; Undefinierte-Variable-Erkennung.

### ✅ WEAK-04: `std::system()` für Kompilierung — Injection-Risiko
- **Datei:** [blitzcc.cpp](file:///g:/dev/projects/bltznxt/src/compiler/blitzcc.cpp)
- **Problem:** Die Compiler-Befehlszeile wird per String-Verkettung gebaut und mit `std::system()` aufgerufen. Dateinamen mit Sonderzeichen (`;`, `&`, `|`) können Shell-Injection verursachen.
- **Behoben:** `std::system()` durch `CreateProcessW()` ersetzt — g++ wird direkt gestartet ohne Shell-Umweg. `WaitForSingleObject` + `GetExitCodeProcess` liefern den Exit-Code. `#include <windows.h>` ans Ende der Includes gesetzt (nach allen Projekt-Headern), damit `windows.h`-Makros (`BOOL`, `ERROR`, `min/max`) nicht in `token.h`/`parser.h` einbluten.

### ✅ WEAK-05: `dimmedArrays` — Dim-Forward-Referenzen nicht erkannt
- **Dateien:** [parser.h](file:///g:/dev/projects/bltznxt/src/compiler/parser.h), [emitter.h](file:///g:/dev/projects/bltznxt/src/compiler/emitter.h)
- **Problem:** `dimmedArrays` wurde während des Parsens erst beim Erreichen des `Dim`-Statements befüllt. Arrays, die vor ihrer `Dim`-Deklaration im Token-Stream genutzt wurden (typisch: Funktion deklariert vor dem Top-Level-`Dim`, oder Include-Struktur mit use-before-include), wurden als Funktionsaufrufe geparst statt als Array-Zugriffe. Außerdem waren Dim'd Arrays als Locals von `main()` deklariert — nicht auf File-Scope — und daher in User-Functions nicht zugänglich.
- **Behoben (Parser):** `preScanDims()` Pre-Scan-Pass in `parser.h` — scannt alle Tokens nach `DIM`-Keywords vor dem eigentlichen Parse-Lauf und registriert alle Array-Namen in `dimmedArrays`. Forward-Referenzen werden korrekt als Array-Zugriffe erkannt.
- **Behoben (Emitter):** `collectDims()` in `emitter.h` — analog zu `collectGlobals()`: deklariert alle Top-Level-Dim-Arrays als leere `std::vector<T>` auf **File-Scope** (vor den User-Functions). `visit(DimStmt*)` emittiert bei bereits gehoistetem Array eine Zuweisung (`var_name = VecType(dims)`) statt einer Neu-Deklaration — Re-Dim bleibt korrekt erhalten.

### ✅ WEAK-06: Gosub verwendet GCC computed-goto Extension
- **Datei:** [emitter.h](file:///g:/dev/projects/bltznxt/src/compiler/emitter.h)
- **Problem:** `&&label` und `goto *ptr` sind GCC/MinGW-spezifisch. Code ist nicht portabel zu MSVC oder anderen Compilern.
- **Behoben:** GCC-Extension durch portable `int __gosub_ret__`-Variable + Dispatch-Switch ersetzt. `GosubStmt` emittiert `__gosub_ret__ = N; goto lbl_X; _gosub_ret_N_:;`. Bare `Return` emittiert `goto __gosub_dispatch__`. Nach `return 0;` in `main()` wird ein Dispatch-Switch (`case N: goto _gosub_ret_N_`) emittiert — nur wenn Gosub-Statements vorhanden sind.

### ✅ WEAK-07: Keine Bounds-Checking bei Array-Zugriff
- **Problem:** `Dim`-Arrays werden als `std::vector` emittiert, und der Zugriff erfolgt per `[index]` ohne Bound-Check. Out-of-Bounds-Zugriffe sind undefined behavior.
- **Behoben:** `[index]` → `.at(index)` in `visit(ArrayAccess*)` und `visit(ArrayAssignStmt*)` in `emitter.h`. Bei Out-of-Bounds wirft `std::vector::at()` `std::out_of_range` mit klarer Fehlermeldung (Index + Arraygröße) statt UB. Gilt für alle Dimensionen (z.B. `var_grid.at(x).at(y)`). Kein Compile-Zeit-Flag nötig.

### ✅ WEAK-08: `bb_file_handles_` lecken bei Programmende
- **Datei:** [bb_file.h](file:///g:/dev/projects/bltznxt/src/compiler/bb_file.h), [bb_bank.h](file:///g:/dev/projects/bltznxt/src/compiler/bb_bank.h)
- **Problem:** Es gibt kein `bb_file_quit_()` in `bbEnd()`. Offene Dateien werden nie automatisch geschlossen. Gleiches gilt für `bb_dir_handles_` und `bb_bank_handles_`.
- **Behoben:** `bb_file_quit_()` in `bb_file.h` — schließt alle offenen `FILE*`-Handles und löscht `bb_dir_handles_`. `bb_bank_quit_()` in `bb_bank.h` — löscht alle Bank-Handles. Beide werden in `bbEnd()` (`bb_runtime.h`) vor `bb_snd_quit_()`/`bb_sdl_quit_()` aufgerufen.

### ✅ WEAK-09: CMakeLists verlinkt SDL3 zur Compile-Tool — nicht zum User-Programm
- **Datei:** [CMakeLists.txt](file:///g:/dev/projects/bltznxt/CMakeLists.txt)
- **Problem:** `target_link_libraries(blitzcc PRIVATE SDL3::SDL3)` verlinkt SDL3 zum Compiler-Binary selbst. Der Compiler braucht kein SDL — nur die generierten Programme. Das ist architektonisch falsch und unnötig.
- **Behoben:** `target_link_libraries(blitzcc PRIVATE SDL3::SDL3)` entfernt. `find_package(SDL3 REQUIRED)` → `find_package(SDL3)` (optional, für informatorische Zwecke). Kommentar erklärt, dass SDL3 nur von den generierten Programmen genutzt wird.

### ✅ WEAK-10: `bb_Int()` Namens-Kollision zwischen `bb_math.h` und `bb_string.h`
- **Dateien:** [bb_math.h](file:///g:/dev/projects/bltznxt/src/compiler/bb_math.h), [bb_string.h](file:///g:/dev/projects/bltznxt/src/compiler/bb_string.h)
- **Problem:** `bb_Int(float)` in `bb_math.h` und `bb_Int(const bbString&)` in `bb_string.h` bilden eine fragile Überladung. In der Praxis war `bb_Int(3.9)` (double-Literal) tatsächlich ambig zwischen `float` und `int`.
- **Behoben:** `bb_Int(float)` → `bb_Int(double)` (akzeptiert double-Literale direkt, float via Promotion) + `bb_Int(int)` (exact match für int-Argumente). Drei überladeungsfreie Kandidaten: `bb_Int(double)`, `bb_Int(int)`, `bb_Int(const bbString&)` — keine Ambiguität mehr.

### ✅ WEAK-11: `bb_DataVal` — theoretische Konstruktor-Ambiguität bei `long`-Literalen
- **Datei:** [bb_runtime.h](file:///g:/dev/projects/bltznxt/src/compiler/bb_runtime.h)
- **Problem:** Alle drei `bb_DataVal`-Konstruktoren sind `explicit`. Würde der Emitter `bb_DataVal(42L)` erzeugen, wäre der Aufruf zwischen `bb_DataVal(int)` und `bb_DataVal(float)` ambig.
- **Behoben:** `explicit bb_DataVal(long v)` hinzugefügt — long-Literale werden als KIND_INT behandelt. Kein Ambiguitäts-Risiko mehr.

### ✅ WEAK-12: `lexString()` — unclosed String erzeugt nur Warning, keinen Fehler
- **Datei:** [lexer.h](file:///g:/dev/projects/bltznxt/src/compiler/lexer.h)
- **Problem:** Trifft `lexString()` auf `\n` ohne schließendes `"`, gab es nur eine `std::cerr`-Warnung (ohne Dateiname, nicht IDE-parseable) und der Parse lief weiter mit einem unvollständigen Token.
- **Behoben:** `Lexer` erhält `filename`-Parameter und `lexErrors_`-Zähler. `lexString()` emittiert GCC-Format-Fehler (`file:line:col: error: unclosed string literal`) und inkrementiert `lexErrors_`. `blitzcc.cpp` übergibt den Dateinamen und prüft `lexer.hasErrors()` — gibt Exit-Code 1 zurück. Negativtest `neg_unclosed_string.bb` hinzugefügt.

---

## 🟠 Redundanz (Redundancies)

### RED-01: `toLower`-Pattern wird >15× inline wiederholt
- **Dateien:** `parser.h`, `emitter.h`
- **Problem:** Das Muster `std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower)` erscheint manuell an über 15 Stellen. Es existiert ein `toUpper()` in `lexer.h`, aber kein entsprechendes `toLower()`.
- **Empfehlung:** `toLower()` utility-Funktion neben `toUpper()` einführen.

### RED-02: Type-Hint-Parsing wird 6× dupliziert
- **Datei:** [parser.h](file:///g:/dev/projects/bltznxt/src/compiler/parser.h)
- **Problem:** Das gleiche Pattern zur Erkennung von `#`, `%`, `!`, `$` Type-Hints wird in `parseVarDecl()`, `parseDim()`, `parseConst()`, `parseFunctionDecl()`, `parseFor()`, `parseRead()` und `parsePrimary()` fast identisch wiederholt.
- **Empfehlung:** `tryConsumeTypeHint()` Helper-Methode einführen.

### RED-03: `inExprCtx` save/restore Boilerplate — >12× wiederholt
- **Datei:** [emitter.h](file:///g:/dev/projects/bltznxt/src/compiler/emitter.h)
- **Problem:** Das Pattern `bool prev = inExprCtx; inExprCtx = true; …; inExprCtx = prev;` wird an fast jeder Visitor-Methode wiederholt. `emitExpr()` kapselt es bereits, wird aber nicht überall genutzt.
- **Empfehlung:** `emitExpr()` konsequent verwenden statt manuelles save/restore.

### RED-04: Goto/Gosub-Parsing fast identisch dupliziert
- **Datei:** [parser.h](file:///g:/dev/projects/bltznxt/src/compiler/parser.h#L129-L152)
- **Problem:** `GOTO` (L129–L140) und `GOSUB` (L141–L152) sind bis auf den AST-Node-Typ zeichenweise identisch.
- **Empfehlung:** In eine gemeinsame `parseLabelJump()` Methode extrahieren.

### RED-05: `StmtNode`-Klasse existiert, wird aber kaum genutzt
- **Datei:** [ast.h](file:///g:/dev/projects/bltznxt/src/compiler/ast.h#L106)
- **Problem:** `StmtNode : ASTNode` und `ExprNode : ASTNode` bilden die Klassenhierarchie, aber der Emitter und Parser arbeiten überwiegend mit `ASTNode*`. Die Trennung ist konzeptionell gut, wird aber nicht enforced — Statements und Expressions können frei gemischt werden.

### RED-06: `Emitter::collectData()` und `collectGlobals()` rekursieren manuell durch den gesamten AST
- **Datei:** [emitter.h](file:///g:/dev/projects/bltznxt/src/compiler/emitter.h#L794-L857)
- **Problem:** Beide Funktionen verwenden lange `dynamic_cast`-Ketten, um durch alle AST-Nodes zu iterieren. Ein generischer AST-Walker (Visitor mit rekursivem Default) würde diese Redundanz eliminieren.

### RED-07: Build-System-Dualität — CMake + bat-Skript
- **Dateien:** `CMakeLists.txt`, `build_windows.bat`
- **Problem:** Zwei separate Build-Systeme existieren parallel. Das bat-Skript baut direkt mit g++, CMakeLists.txt verlinkt unnötig SDL3 zum Compiler. Inkonsistenz im Build-Prozess.

### ✅ RED-08: `bb_ReadAvail()` dupliziert den gesamten `fseek`/`ftell`-Block von `bb_Eof()`
- **Datei:** [bb_file.h:89-97](file:///g:/dev/projects/bltznxt/src/compiler/bb_file.h#L89)
- **Problem:** `bb_ReadAvail()` enthält zeichenweise denselben `ftell` → `fseek(SEEK_END)` → `ftell` → `fseek(cur)` Block wie `bb_Eof()`. Die Logik zur Positionsermittlung müsste in eine gemeinsame Hilfsfunktion `bb_file_size_remaining_()` extrahiert werden.
- **Hinweis:** Erbt damit auch das unter BUG-05 beschriebene Text-Mode-Problem auf Windows.
- **Behoben:** Im Zuge von BUG-05 — `bb_file_remaining_(FILE*)` als interner Helper eingeführt; `bb_ReadAvail()` delegiert dorthin.

---

## Zusammenfassung

| Kategorie     | Anzahl | Änderung |
|:-------------|:------:|:--------|
| 🔴 Bugs       |   7    | BUG-05 Beschreibung korrigiert; BUG-08 → WEAK-11 |
| 🟡 Schwachstellen | 12 | WEAK-05 ✅; +WEAK-11 (ehem. BUG-08, theoretisch), +WEAK-12 (Lexer-String) |
| 🟠 Redundanz   |   8    | +RED-08 (bb_ReadAvail Duplikat) |
| **Gesamt**     | **27** | |

> ⚠️ **Kritischste Funde:** BUG-01 (SHR/SAR), BUG-03/04 (Rand UB), BUG-05 (Eof Text-Mode), BUG-06 (fehlende Hex/Bin-Literale), BUG-07 (String-Escaping), WEAK-03 (fehlender Semantic-Pass), WEAK-12 (unclosed String silent failure).
