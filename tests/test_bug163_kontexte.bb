; BUG-163 - Registerwerte der Mathefunktionen in allen Ausdruckszusammenhaengen:
; Verkettung, Mod, ^, Int, Abs, Select/Case, Parameter, Felder, Arrays, Bit-
; operatoren, Bedingung, Vergleich mit String. "Sin(30) And 1" ist 1, weil die
; Ganzzahl-Rundung den ungerundeten Wert 0.50000001 nimmt. Am Original gemessen.

Type T
Field f#
Field i
End Type
Function G#(x#)
Return Sin(x)
End Function
Function H(x)
Return Cos(x)
End Function
Local a# = 30
Local t.T = New T
t\f = Sin(a)
t\i = Cos(a) * 100
Dim arr#(3)
arr(1) = Sin(a)
Local v#[2]
v[0] = Cos(a)
Print "s" + Sin(a)
Print Sin(a) + "s"
Print Sin(a) Mod 0.3
Print Sin(a) ^ 2
Print Int(Sin(a) * 10)
Print Abs(Sin(-a))
Print Sgn(Sin(-a))
Print -Sin(a)
Print Sin(a) And 1
Print Not Sin(a)
Print Sin(a) / 2
Print G(a) + H(a)
Select Sin(90)
Case 1
Print "eins"
End Select
Select 1
Case Sin(90)
Print "auch"
End Select
Print (Sin(a) <> 0) + 10 * (Int(Sin(a)))
Print (Sin(a) > "0.4")
For i = 0 To Sin(90) * 3
Next
Print i
Local s$ = Sin(a)
Print s
Print Rnd(Sin(a), 1) >= 0
Print Floor(Sin(a) * 10)
Print Sqr(Sin(a))
Print Sin(Sin(a))
Print t\f + t\i + arr(1) + v[0]
Print Float(Sin(a))
Print Str(Cos(a))
Print Sin(a) = Cos(60)
Print Int(Sin(30))
Print Int(Cos(60))
Local k% = Sin(30)
Print k
Print Sin(30) Or 0
Print (Sin(30) * 1) And 1
