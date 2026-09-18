; BUG-85 (Rest) - ein schlichtes Read nimmt den Typ der deklarierten Variablen,
; nicht den (fehlenden) Tag an der Read-Stelle: lokal, global, Parameter,
; implizit getaggt, in einer Liste. Am Original gemessen (build/read20260918).
; Am Ende: Read hinter dem letzten Data-Wert bricht ab wie im Original
; ("Out of data"), die Zeile "continued" darf nicht erscheinen.

Global g# = 0
Global gs$ = ""
Function F(p#, q$)
	Read p
	Read q
	Print "param " + Int(p * 10) + " " + q
	Local lf#
	Read lf
	Print "localfn " + Int(lf * 10)
	Read g
	Print "globalfn " + Int(g * 10)
End Function
Data 1.5, "ABC", 2.5, 7, 3.7, "9.5", 4.5, 2.5, 3.5, "x", 5.5, 6.5, 8.5, 12, 1.5
Local x#
Read x
Print "localf " + Int(x * 10)
Local s$
Read s
Print "locals " + s
Read g
Print "global " + Int(g * 10)
Read gs
Print "globals " + gs
Local i%
Read i
Print "localint " + i
Read s
Print "strfromstr " + s
F(0, "")
Read s
Print "strfromnum " + s
y# = 0
Read y
Print "implicit " + Int(y * 10)
Read z
Print "neu " + z
Read x, s
Print "liste " + Int(x * 10) + " " + s
Print "ende"
Read w
Print "continued"
