; BUG-107 - Data nimmt konstante Ausdruecke wie im Original
; (DataDeclNode::proto): gefaltet wie ein Const, der Typ des Ausdrucks bleibt
; erhalten, gewandelt wird erst beim Read. Ein Const darf auch erst hinter
; der Data-Zeile stehen. Die Erwartung stammt vom Original
; (build/data20260926).
Data K2, K2*2
Read a, b
Print a
Print b
Const K2=5

Const N=2, F#=1.5, S$="x"
.werte
Data N+1, Pi, True, -N, "a"+"b", 7/2, 7/2.0, 2^3, "x"+N, N*1.5
Data Abs(-3), Int(2.5), Str(1.5), Not 0, 1 Shl 3, $10, %11, "a"<"b", Float(3), -2.5
Data F, S, F*2, S+F, -(3), +4, 1.5 And 3, 10 Mod 3, 2147483647+1, "12"+1
For i=1 To 30
Read ss$
Print ss
Next
Restore werte
For i=1 To 30
Read ff#
Print ff
Next
Restore werte
For i=1 To 30
Read kk
Print kk
Next

.klammern
Data (1+2)*3, ((4)), 1+1, 2 : Read a : Print a
Restore klammern
Read a, b, c, d
Print a : Print b : Print c : Print d
