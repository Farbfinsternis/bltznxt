; BUG-73 - Mod ist in Blitz3D nicht auf Ganzzahlen beschraenkt.
;
; Referenz: ArithExprNode::translate in compiler/exprnode.cpp waehlt zwischen
; zwei Laufzeitfunktionen - "__bbMod" bei zwei Ints, "__bbFMod" sobald eine
; Seite ein Float ist. bbruntime/basic.cpp definiert sie als "return x%y" und
; "return (float)fmod(x,y)". ArithExprNode::semant zieht dabei BEIDE Seiten
; auf Float, wenn eine es ist - ein gemischtes Mod ist also ein Float-Mod.
;
; Alle erwarteten Werte sind am laufenden Original gemessen (2026-09-10,
; dasselbe Programm mit WriteFile statt Print). Die Float-Ergebnisse gehen
; mal 1000 durch Int(), damit BUG-68 (Str(float) formatiert anders) das
; Ergebnis nicht verdeckt.

; --- Konstanten: die faltet das Original zur Uebersetzungszeit, mit
;     denselben zwei Operationen. Compilezeit und Laufzeit muessen daher
;     dasselbe liefern.
Print "K int   7 Mod 3   = " + Str(7 Mod 3)
Print "K int  -7 Mod 3   = " + Str(-7 Mod 3)
Print "K int   7 Mod -3  = " + Str(7 Mod -3)
Print "K int  -7 Mod -3  = " + Str(-7 Mod -3)
Print "K flt   7.5 Mod 2 = " + Str(Int((7.5 Mod 2) * 1000))
Print "K flt  -7.5 Mod 2 = " + Str(Int((-7.5 Mod 2) * 1000))
Print "K flt   7.5 Mod -2= " + Str(Int((7.5 Mod -2) * 1000))
Print "K mix   7 Mod 2.5 = " + Str(Int((7 Mod 2.5) * 1000))
Print "K mix   7.5 Mod 3 = " + Str(Int((7.5 Mod 3) * 1000))

; --- Variablen: der Laufzeitpfad. Das Vorzeichen ist das des Dividenden,
;     bei beiden Operationen.
a% = 7 : b% = 3 : c% = -7 : d% = -3
Print "V int   a Mod b   = " + Str(a Mod b)
Print "V int   c Mod b   = " + Str(c Mod b)
Print "V int   a Mod d   = " + Str(a Mod d)
Print "V int   c Mod d   = " + Str(c Mod d)

x# = 7.5 : y# = 2.0 : z# = -7.5 : w# = -2.0
Print "V flt   x Mod y   = " + Str(Int((x Mod y) * 1000))
Print "V flt   z Mod y   = " + Str(Int((z Mod y) * 1000))
Print "V flt   x Mod w   = " + Str(Int((x Mod w) * 1000))

; --- gemischt: eine Float-Seite genuegt, das Ergebnis ist ein Float
Print "V mix   a Mod y   = " + Str(Int((a Mod y) * 1000))
Print "V mix   x Mod b   = " + Str(Int((x Mod b) * 1000))
Print "V mix   c Mod y   = " + Str(Int((c Mod y) * 1000))

; --- ein Int-Mod bleibt ganzzahlig, ein Float-Mod behaelt die Nachkommastellen
e# = 7.9
Print "V int-erg a Mod b *1000 = " + Str(Int((a Mod b) * 1000))
Print "V trunk   e Mod y       = " + Str(Int((e Mod y) * 1000))
