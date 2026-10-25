; BUG-80 - auch der Wert einer Objektvariable passt nicht in eine Variable
; ohne Tag. Das Original meldet "Illegal type conversion" (gemessen, 11.8).
; Vorher merkten wir uns fuer c den Objekttyp, ein spaeteres c\q ging durch.
Type T
	Field q
End Type
r.T = New T
c = r
Print 1
