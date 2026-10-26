; BUG-160 - ein For-Zaehler darf ein Element eines festen Arrays oder eine Feldkette sein.
;
; Das Original liest den Zaehler mit parseVar() wie jedes Ziel einer
; Zuweisung. Gemessen an Blitz3D 11.8 (build/bug160_20261025/forz): genau diese
; Ausgabe. Vorher lehnte unser Parser "For v[0] = ..." mit "Expected =" ab
; und kannte hinter dem Namen nur ein einzelnes Feld.
Type Innen
	Field k
	Field w#[2]
End Type
Type Aussen
	Field i.Innen
End Type
Local v%[2]
For v[0] = 0.6 To 2.5
	Print "v " + v[0]
Next
Print "nach " + v[0]
a.Aussen = New Aussen
a\i = New Innen
For a\i\k = 1 To 3
	Print "k " + a\i\k
Next
For a\i\w[1] = 0 To 1 Step 0.5
	Print "w " + a\i\w[1]
Next
Dim d.Innen(2)
d(1) = New Innen
For d(1)\k = 5 To 6
	Print "d " + d(1)\k
Next
