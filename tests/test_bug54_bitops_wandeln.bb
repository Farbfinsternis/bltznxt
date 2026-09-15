; BUG-54 - And, Or, Xor, Shl, Shr und Sar wandeln beide Seiten nach int.
;
; In compiler/exprnode.cpp sind diese sechs BinExprNode, nicht ArithExprNode:
; BinExprNode::semant ruft castTo(int_type) fuer beide Seiten. Ein String
; wandelt dabei mit atoi (__bbStrToInt), ein Float rundet auf die naechste
; gerade Zahl - dieselbe CastNode wie in einer Bedingung (BUG-61).
; Bis 2026-09-15 lehnten wir Strings ab ("Operator cannot be applied to
; strings"), und ein Float kam bis g++ ("invalid operands of types 'float'
; and 'int'").

; --- 1) Der gemessene Fall aus den Beispielprogrammen ---
; Liest sich wie "weder yes noch no", ist aber (q$ <> "yes") Or atoi("no").
q$ = "maybe"
If q$ <> "yes" Or "no" Then Print "oder mit string: ja"

; --- 2) Strings: atoi ---
Print "12abc" And 7
Print "3" Or 8
Print "5" Xor 1
Print "abc" Or 0

; --- 3) Floats: auf gerade gerundet ---
x# = 2.5
Print x And 3
y# = 3.5
Print y Or 0
Print 7 Shl 1.6
Print -16 Sar 1.4
Print -16 Shr 28

; --- 4) Ganzzahlen und Vergleiche wie bisher ---
a = 6
b = 3
Print a And b
Print (a > 1) And (b > 1)
If Not (a = 1) And b Then Print "not bindet am lockersten"

; --- 5) Als konstanter Ausdruck ---
Const MASKE = 12 Or 3
Local feld[MASKE And 7]
Print "maske " + MASKE
