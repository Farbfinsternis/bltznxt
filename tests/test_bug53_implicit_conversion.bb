; BUG-53 - Zahl und String wandeln an jeder Zuweisungsgrenze ineinander um.
;
; Am laufenden Original gemessen (Blitz3D 11.8): Integer, Float und String
; wandeln in ALLEN sechs Richtungen, und zwar an vier Stellen mit bekanntem
; Zieltyp - Zuweisung, Local/Global mit Initialisierung, Funktionsparameter und
; Return. Objekte wandeln nie ("Illegal type conversion").
;
; Vorher lehnte unser Analyzer das ab und war damit strenger als die Sprache:
; 31 der 61 Befunde im Beispielbestand waren genau dieser Fehler, fast immer in
; der Form "Text 10,20,punkte" mit einem Integer als String-Parameter.
;
; String -> Zahl folgt der Referenz, die dafuer atoi/atof benutzt: der fuehrende
; Zahlanteil zaehlt, der Rest wird ignoriert, gar keine Ziffer ergibt 0.

; --- 1) Zahl -> String
Local s1$ = 42
Print s1
Local s2$ = -7
Print s2

Local s3$ = ""
s3 = 99
Print s3

Global g$ = 1234
Print g

; --- 2) String -> Zahl, mit den Randfaellen von atoi/atof
Local n1% = "12"
Print n1
Local n2% = "abc"
Print n2
Local n3% = "12abc"
Print n3
Local f1# = "1.5"
Print f1 * 2

Local n4% = 0
n4 = " 7 "
Print n4

; --- 3) Parameter in beide Richtungen
Function AlsText(s$)
  Return Len(s)
End Function
Print AlsText(1234)

Function AlsZahl(n%)
  Return n + 1
End Function
Print AlsZahl("41")

; --- 4) Return in beide Richtungen
Function GibText$()
  Return 7
End Function
Print GibText() + "!"

Function GibZahl%()
  Return "41"
End Function
Print GibZahl() + 1

; --- 5) Diese beiden Zeilen standen bis zum 2026-09-08 als NEGATIVtests im
;        Bestand (neg_weak17_builtin_return / neg_weak17_builtin_types). Das
;        Original nimmt beide an - sie hielten unsere eigene zu strenge Regel
;        fest, nicht Blitz3D. Astra hatte genau darauf hingewiesen.
Local s4$ = Len("abc")
Print s4
Print Sin("x")
