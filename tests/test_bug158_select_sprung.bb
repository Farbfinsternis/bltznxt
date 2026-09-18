; BUG-158 - Goto oder Gosub-Ruecksprung in einen Case-Zweig.
;
; Ein Gosub im Zweig kehrt per Sprung dorthin zurueck; ueber einen
; "auto _sel_ = ..." darf C++ nicht springen. Select ueber zusammengesetzte
; Ausdruecke, Strings, Felder und Aufrufe, dazu ein Goto in einen Zweig in einer
; Funktion. Am Original gemessen (2026-09-18, build/sel20260918/).

Type tDing
	Field wert#
	Field name$
End Type

Global d.tDing = New tDing
d\wert = 2.5
d\name = "b"

Function Doppelt(x)
	Return x * 2
End Function

Function MitLocal$()
	Local s$ = "text"
	Return s
End Function

Function ImplizitInt()
	s = 7
	n = 0
	Select s
		Case 7
			Ausgabe "f: case 7"
.drin
			n = n + 1
			Ausgabe "f: drin " + n
		Default
			Ausgabe "f: default"
	End Select
	If n < 2 Then Goto drin
	Return s
End Function

Function Ausgabe(t$)
	Print t
End Function

For i = 1 To 3
	Select i * 2 + 1
		Case 3
			Gosub unter
			Ausgabe "ausdruck 3 nach gosub"
		Case 5
			Ausgabe "ausdruck 5"
		Default
			Ausgabe "ausdruck default " + i
	End Select
Next

s$ = "b"
Select s + ""
	Case "a" : Ausgabe "string a"
	Case "b" : Gosub unter : Ausgabe "string b nach gosub"
End Select

Select d\wert
	Case 2.5 : Gosub unter : Ausgabe "feld 2.5 nach gosub"
End Select

Select d\name
	Case "b" : Gosub unter : Ausgabe "feldname b nach gosub"
End Select

Select Doppelt(3)
	Case 6 : Gosub unter : Ausgabe "aufruf 6 nach gosub"
End Select

Select Doppelt(2) = 4
	Case True : Gosub unter : Ausgabe "vergleich wahr nach gosub"
End Select

Ausgabe MitLocal()
Ausgabe "rueckgabe " + ImplizitInt()
End

.unter
Ausgabe "  im gosub"
Return
