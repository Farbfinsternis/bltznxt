; BUG-42 - die Gegenprobe: was an Marken und Sprungzielen gueltig bleibt.
;
; Nachgeschlagen in compiler/parser.cpp und compiler/stmtnode.cpp (2026-09-15):
; Goto, Gosub und Restore lesen ihr Ziel mit parseIdent() bzw. pruefen auf
; IDENT - ohne Punkt. Nur die Definition ".name" hat einen. Die
; Ablehnungsfaelle liegen als neg_bug42_* daneben.

; --- 1) Dieselbe Marke im Hauptprogramm und in einer Funktion ---
; Marken sind funktionslokal (BUG-87); die Pruefung auf doppelte Marken
; gilt je Bereich.
Function Zaehle()
  Local i = 0
.nochmal
  i = i + 1
  If i < 3 Then Goto nochmal
  Return i
End Function

Print "funktion " + Zaehle()
Goto nochmal
Print "FEHLER uebersprungen"
.nochmal
Print "hauptprogramm"

; --- 2) Restore mit Punkt ---
; Die Referenz liest "Restore .zweite" als Restore ohne Ziel und danach die
; Definition der Marke ".zweite". Es gibt sie hier nur dieses eine Mal, also
; ist das Programm gueltig - und Read beginnt wieder beim ersten Data.
Data 1
Data 2
Read a
Read b
Restore .zweite
Read c
Print "restore " + a + " " + b + " " + c
