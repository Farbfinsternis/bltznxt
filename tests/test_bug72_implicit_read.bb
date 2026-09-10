; BUG-72 - eine nie deklarierte Variable entsteht auch beim blossen LESEN.
;
; Referenz: IdentVarNode::semant in compiler/varnode.cpp sucht den Namen mit
; findDecl() und legt ihn, wenn nichts gefunden wird, mit seinem "ugly auto
; decl!" als DECL_LOCAL im Environ des umgebenden Rumpfes an. Lesen und
; Schreiben gehen durch dieselbe Stelle; der Typ folgt dem Tag, ohne Tag ist
; es ein Int.
;
; Die erwarteten Werte sind am laufenden Original gemessen (2026-09-10,
; dasselbe Programm mit WriteFile statt Print), nicht abgeleitet.

Const KONST% = 7
Global glob% = 11

Print "str=[" + nie_str$ + "]"
Print "int=" + Str(nie_int)
Print "flt=" + Str(Int(nie_flt#))
Print "summe=" + Str(nie_int + KONST + glob)

; Ein Funktionsname ohne Klammern ist kein Aufruf, sondern eine implizite
; Variable: findDecl durchsucht decls, Funktionen liegen in funcDecls.
; Am Original gemessen: 0, nicht 99.
Print "funcname=" + Str(hilf)

haupt_var = 5
zeige()
Print "haupt danach=" + Str(haupt_var)

Function hilf%()
	Return 99
End Function

Function zeige()
	; Der Gueltigkeitsbereich ist der Rumpf: haupt_var aus dem Hauptteil ist
	; hier nicht sichtbar, es entsteht eine eigene lokale Variable. Die
	; Zuweisung unten laesst den Hauptteil deshalb unberuehrt.
	Print "in funktion haupt_var=" + Str(haupt_var)
	; Konstante und Global duerfen dabei NICHT verschattet werden - genau das
	; waere der stille Fehler, wenn das Hoisten der impliziten Variablen sie
	; mitnaehme.
	Print "in funktion KONST=" + Str(KONST)
	Print "in funktion glob=" + Str(glob)
	Print "in funktion frisch=[" + frisch$ + "]"
	haupt_var = 77
End Function
