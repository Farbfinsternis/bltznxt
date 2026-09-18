; BUG-157 - eine Zuweisung an ein Feld wandelt auf den Typ des Feldes.
;
; String in Integer- und Float-Feld, Zahl in String-Feld, Float in Integer-Feld,
; ueber ein globales, ein lokales, ein verschachteltes, ein Array- und ein
; Parameterobjekt. Am Original gemessen (2026-09-18, build/fld20260918/).
; Float-Felder werden als Int(f * 100) ausgegeben, weil Str(float) noch
; abweicht (BUG-68); Rundung beim Wandeln nach Int ist BUG-95.

Type tInner
	Field i%
	Field f#
	Field s$
End Type

Type tOuter
	Field kind.tInner
	Field n
End Type

Global g.tInner = New tInner
Dim arr.tInner(2)
arr(1) = New tInner

Function Zeige$(x.tInner)
	Return x\i + " " + Int(x\f * 100) + " " + x\s
End Function

; global object
g\i = "42"
g\f = "1.25"
g\s = 17
Ausgabe "global " + Zeige(g)
g\i = 3.25
g\f = 7
g\s = 0.5
Ausgabe "global2 " + Zeige(g)

; local object
l.tInner = New tInner
l\i = "12abc"
l\f = 9
l\s = 2.5
Ausgabe "lokal " + Zeige(l)

; nested
o.tOuter = New tOuter
o\kind = New tInner
o\kind\i = "7"
o\kind\f = "0.75"
o\kind\s = 99
o\n = "5"
Ausgabe "verschachtelt " + Zeige(o\kind) + " n " + o\n

; array
arr(1)\i = "8"
arr(1)\s = 1.5
Ausgabe "array " + Zeige(arr(1))

; in a function, parameter object
Setze(l)
Ausgabe "funktion " + Zeige(l)
End

Function Setze(x.tInner)
	x\i = "31"
	x\s = x\i + 1
	x\f = x\s
End Function

Function Ausgabe(t$)
	Print t
End Function
