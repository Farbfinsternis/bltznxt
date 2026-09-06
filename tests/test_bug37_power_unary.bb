; BUG-37: In der Referenz nimmt parseExpr6 seine Operanden von parseUniExpr.
; Das Vorzeichen liegt damit *unterhalb* von "^" und gehoert zur Basis:
; -2 ^ 2 ist (-2) ^ 2 = 4, nicht -(2 ^ 2) = -4.

Print -2 ^ 2
Print -2 ^ 3
Print 2 ^ -1
Print ~2 ^ 2

; Die seit BUG-36 unaeren Formen liegen auf derselben Ebene.
Print Abs -2 ^ 2

; "^" bindet weiterhin staerker als "*", und bleibt linksassoziativ (BUG-22).
Print 3 * 2 ^ 2
Print 2 ^ 2 * 3
Print 2 ^ 3 ^ 2

Local a# = 4.0
Print -a ^ 2

; Klammern bleiben, was sie waren.
Print -(2 ^ 2)
