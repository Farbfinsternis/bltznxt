; BUG-100: Ein Local in einer Funktion verdeckt ein Global gleichen Namens -
; ab seiner Stelle im Quelltext bis zum Ende des Rumpfs, auch hinter dem
; Block, in dem es steht. Davor (und im eigenen Initialisierer) gilt noch das
; Global, auch wenn eine Schleife oder ein Goto spaeter wieder dorthin kommt.
; Erwartung am Original Blitz3D V11.8 gemessen (2026-10-13).
Type T
	Field v
End Type
Global x = 1
Global s = 7
Global o.T = New T
o\v = 100
Global cnt
Global k = 9
Global r = 3
Global p.T = New T
p\v = 1
Global e.T
Global z# = 0.5
Data 77, "acht"
Function rd()
	If 1 Then
		Local r$
	EndIf
	Read r
	Read r
	Print r + "!"
End Function
Function fr()
	Print k
	Repeat
		Local k = 0
	Until 1
	For k = 1 To 3
	Next
	Print k
End Function
Function fe()
	Local q.T = New T
	q\v = 2
	If p <> Null Then
		Local e.T
	EndIf
	n = 0
	For e = Each T
		n = n + e\v
	Next
	Print n
	Print e = Null
End Function
Function dl()
	Select 1
		Case 1
			Local p.T = First T
	End Select
	Delete p
	Local cnt = 0
	For y.T = Each T
		cnt = cnt + 1
	Next
	Print cnt
End Function
Function fl()
	Local z = 7.6
	Print z
	z = z / 2
	Print z
End Function
Function h()
	Local x = x + 5
	Print x
End Function
Function loopf()
	For i = 1 To 3
		Print x
		Local x = 5 + i
		Print x
	Next
	Print x
End Function
Function gotof()
	n = 0
.oben
	n = n + 1
	Print x
	Local x = 40 + n
	If n < 2 Then Goto oben
	Print x
End Function
Function typf()
	Print s
	If s Then
		Local s$ = "a"
	EndIf
	Print s + "b"
End Function
Function objf()
	Print o\v
	While cnt < 1
		cnt = cnt + 1
		Local o.T = New T
		o\v = 5
	Wend
	Print o\v
End Function
Function sel(k)
	Select k
		Case 1
			Local x# = 2.5
	End Select
	Print x
End Function
loopf()
Print x
gotof()
Print x
typf()
Print s
objf()
Print o\v
sel(1)
sel(2)
Print x
h()
Print x
rd()
Print r
fr()
Print k
fe()
Print e = Null
dl()
fl()
Print z
