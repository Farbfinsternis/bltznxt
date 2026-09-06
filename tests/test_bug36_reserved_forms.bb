; BUG-36: Abs, Sgn, Int, Float und Str sind reservierte Woerter, keine Befehle.
; parseUniExpr in der Referenz baut UniExprNode (Abs/Sgn) bzw. CastNode
; (Int/Float/Str) ueber den *folgenden unaeren Ausdruck* - Klammern sind nicht
; noetig, und ein Type-Tag direkt hinter dem Wort wird gelesen und verworfen.

Print Abs -3
Print Abs(-3)
Print Sgn -2.5
Print Str 42
Print Str$ 42
Print Int "7"
Print Int 7.9
Print Int% 7.9
Print Float "2.5"

; Der Operand ist nur der unaere Ausdruck: das + steht ausserhalb.
Print Abs -3 + 1

Print Abs(Sgn(-4))

Local a% = Abs -7
Print a
Print "wert: " + Str 5
