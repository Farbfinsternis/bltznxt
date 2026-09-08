; BUG-48 - "Then" ist optional, und der einzeilige Rumpf reicht bis Zeilenende.
;
; Referenz gemessen an Blitz3D 11.8 (G:\dev\Blitz3D), nicht nur nachgelesen.
; Zwei Regeln, beide am Original abgelesen:
;
; 1) "Then" entscheidet nichts. Die Form haengt allein am Token direkt nach der
;    Bedingung (und nach einem etwaigen "Then"): Zeilenumbruch oder Doppelpunkt
;    beginnen die Blockform, alles andere die einzeilige. Deshalb verlangt
;    "If a=1 : Print "x"" ein EndIf, "If a=1 Print "x"" dagegen nicht - beides
;    gemessen.
;
; 2) Der einzeilige Rumpf laeuft bis zum Zeilenende, Doppelpunkte eingeschlossen.
;    Im Assembler des Originals ueberspringt bei "If a=1 Print "x" : Print "y""
;    ein einziger bedingter Sprung *beide* Prints, und in
;    "If a=1 P"x" Else P"y" : P"z"" enthaelt der Else-Zweig beide Anweisungen.
;    Der Doppelpunkt trennt hier, er beendet nicht. Vorher las der Parser genau
;    eine Anweisung und liess den Rest der Zeile unbedingt laufen - ein stilles
;    Falschergebnis, das dieser Test absichert.
;
; "End" bleibt bewusst als einzeiliger Rumpf zulaessig ("If a=1 End" nimmt das
; Original an, _fend liegt dort im bedingten Zweig).

; --- 1) einzeilig ohne Then, wahr und falsch
a = 1
If a = 1 Print "eins"
If a = 2 Print "zwei"

; --- 2) einzeilig ohne Then mit Else
If a = 2 Print "dann" Else Print "sonst"

; --- 3) der Rumpf reicht ueber den Doppelpunkt hinaus
b = 0
If b = 1 Print "nicht-a" : Print "nicht-b"
Print "nach-b"

c = 1
If c = 1 Print "ja-a" : Print "ja-b"

; --- 4) auch der Else-Zweig reicht bis Zeilenende
If b = 1 Print "x" Else Print "sonst-a" : Print "sonst-b"

; --- 5) einzeiliges ElseIf
d = 2
If d = 1 Print "d1" ElseIf d = 2 Print "d2"

; --- 6) leerer Then-Zweig mit Else
If b = 1 Else Print "leerer-then"

; --- 7) einzeilig verschachtelt
If a = 1 If c = 1 Print "verschachtelt"

; --- 8) Blockform ohne Then, mit Else und ElseIf
If d = 1
  Print "block-d1"
ElseIf d = 2
  Print "block-d2"
Else
  Print "block-sonst"
EndIf

; --- 9) Doppelpunkt nach der Bedingung ist die Blockform und braucht EndIf
If a = 1 : Print "doppelpunkt-block" : EndIf

Print "ende"
