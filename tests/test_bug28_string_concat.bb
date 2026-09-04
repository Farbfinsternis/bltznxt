; BUG-28 — String-Literale werden als bbString emittiert, nicht als const char*.
; Vorher war "text" + n Zeigerarithmetik auf dem Literal: das erste Zeichen fiel
; weg, die Zahl fehlte, und bei grossem n las das Programm hinter das Literal.
; Blitz3D macht bei einem "+" mit String auf einer Seite den ganzen Ausdruck zum
; String und wandelt die andere Seite um (compiler/exprnode.cpp).

Local n% = 7
Local f# = 1.5
Local s$ = "text: "

Print s + n
Print "literal: " + n
Print "literal: " + Str(n)
Print "a" + "b"
Print "float: " + f
Print n + " nachgestellt"
Print "gemischt " + n + " und " + f

; unveraendert: die uebrigen String-Funktionen und Vergleiche
Print Len("abc")
Print Upper("abc") + Lower("DEF")
Print Mid("hallo", 2, 3)
Local t$ = "x" + "y"
Print t
If "a" = "a" Then Print "vergleich ok"

; Data-Strings laufen ueber einen eigenen Pfad und bleiben unberuehrt
Data "aus data"
Local d$
Read d$
Print d + "!"
