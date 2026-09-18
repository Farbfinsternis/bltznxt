; BUG-36: Abs, Sgn, Int, Float und Str sind reservierte Woerter, keine Befehle.
; Hinweis (2026-09-18): Abs/Sgn liefern hier noch nicht den Typ ihres
; Arguments (BUG-96); seit BUG-68 zeigt die .expected das sichtbar ("3.0"
; statt "3", "-1" statt "-1.0"). Mit BUG-96 wird sie korrigiert.
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
