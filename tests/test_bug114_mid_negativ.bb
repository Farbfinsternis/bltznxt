; BUG-114 - Mid mit negativer Laenge liefert den Rest ab der Startposition.
;
; Referenz: bbMid in bbruntime/bbstring.cpp - "if( n>=0 ) substr( o,n ) else
; substr( o )"; die Vorgabe fuer die Laenge ist -1. Eine ausdrueckliche -1
; muss also dasselbe liefern wie die Form mit zwei Argumenten, und jede andere
; negative Laenge ebenso. Laenge 0 bleibt leer. Alle Werte sind am laufenden
; Original gemessen (2026-09-16).

Print "[" + Mid("abcd", 2, -1) + "]"
Print "[" + Mid("abcd", 2) + "]"
Print "[" + Mid("abcd", 2, -5) + "]"
Print "[" + Mid("abcd", 1, -1) + "]"
Print "[" + Mid("abcd", 2, 0) + "]"
Print "[" + Mid("abcd", 3, 10) + "]"
Print "[" + Mid("abcd", 4, 1) + "]"
Print "[" + Mid("abcd", 5) + "]"
Print "[" + Mid("abcd", 5, 2) + "]"
Print "[" + Mid("abcd", 9) + "]"
Print "[" + Mid("abcd", 9, -1) + "]"

; Die Laenge aus einer Variablen: -1 heisst "bis zum Ende"
n = -1
Print "[" + Mid("Hallo Welt", 7, n) + "]"
