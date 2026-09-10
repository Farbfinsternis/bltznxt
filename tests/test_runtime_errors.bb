; BUG-82: Int() und Float() auf einer Zeichenkette sind DIESELBE Umwandlung
; wie an einer Zuweisungsgrenze - atoi und atof, ohne Ausnahme und ohne
; Meldung. Bis zum 2026-09-10 stand hier "Fallback auf 0 + stderr-Warnung":
; die Warnung ist weg, weil das Original keine kennt und sie bei gueltigen
; Programmen erschien (die BirdDemo erzeugte 42 solcher Zeilen).
;
; Alle Werte unten sind am laufenden Original gemessen (2026-09-10). Sie
; sichern mehr als die alte Zusage: std::stoi und std::stof lagen bei den
; Ueberlauf-, Hex- und "nan"/"inf"-Faellen auch im WERT daneben.
;
; Floats gehen durch Int(), damit BUG-68 (Str(float) formatiert anders) das
; Ergebnis nicht verdeckt.

; --- der alte Kern: kein Absturz, Rueckfall auf 0
Local i% = Int("abc")
Print i                 ; -> 0

Local f# = Float("xyz")
Print f                 ; -> 0. Das Original schreibt hier "0.0" - das ist
                        ;    BUG-68 (Str(float) formatiert anders) und die
                        ;    EINZIGE der 17 Zeilen, die abweicht.

; --- Ueberlauf laeuft um, er klemmt nicht und wirft nicht.
;     std::stoi warf hier und lieferte 0.
Print Int("99999999999")            ; -> 1215752191
Print Int("-99999999999")           ; -> -1215752191
Print Int("2147483648")             ; -> -2147483648
Print Int("4294967296")             ; -> 0

; --- fuehrender Zahlanteil zaehlt, der Rest wird ignoriert
Print Int("-12abc")
Print Int("  12  ")
Print Int("3.9")
Print Int("- 5")                    ; Vorzeichen und Ziffer duerfen nicht
                                    ; durch ein Leerzeichen getrennt sein

; --- die C89-Grammatik: kein Hex, kein "inf", kein "nan"
Print Int(Float("0x10") * 100)
Print Float("nan") = Float("nan")   ; kein NaN, also gleich sich selbst
Print Float("inf") > 1000000.0
Print Int(Float(".5") * 1000)
Print Int(Float("-.5") * 1000)
Print Int(Float("1.5e2"))
Print Float("1e40") > 1000000.0     ; hier wird es unendlich, stof warf
