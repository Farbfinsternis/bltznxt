; BUG-96 - Abs und Sgn sind Operatoren: das Ergebnis hat den Typ des
; Operanden (int oder float), wie UniExprNode in compiler/exprnode.cpp.
; Abs(3)/2 ist 1, Sgn(2.0)/2 ist 0.5, Abs(-2147483648) bleibt int.
; Am Original gemessen (build/abs20260918).

Function IF1%()
	Return -7
End Function
Function FF1#()
	Return -7.5
End Function
Print Abs(3) / 2
Print Abs(-3)
Print Abs(-3.5)
Print Abs(3.0) / 2
Print Sgn(2.0) / 2
Print Sgn(-5) / 2
Print Sgn(-5)
Print Sgn(-2.5)
Print Sgn(0.0)
Print Sgn(0)
Local i% = -9
Local f# = -9.25
Print Abs(i)
Print Abs(f)
Print Sgn(i)
Print Sgn(f)
Print Abs(i) / 2
Print Abs(f) / 2
Print Abs(IF1())
Print Abs(FF1())
Print Abs(-2147483647 - 1)
Local s$ = Abs(-3)
Print s
s = Sgn(-3.5)
Print s
Local g# = Abs(-3) / 2
Print g
Print Abs -3 + 1
Print Abs(i * 2) + Sgn(f * 2)
Print Abs(7 / 2)
Print Abs(7 / 2.0)
Print Abs(-0.0)
Print Sgn(0.1) * 3
Print Abs(1 + 0.5)
