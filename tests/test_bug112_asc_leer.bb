; BUG-112 - Asc eines leeren Strings liefert -1, nicht 0.
;
; Referenz: bbAsc in bbruntime/bbstring.cpp - "s->size() ? (*s)[0] & 255 : -1".
; Damit bleibt der leere String vom Zeichen mit dem Bytewert 0 unterscheidbar.
; Alle Werte sind am laufenden Original gemessen (2026-09-16).

Print Asc("")
Print Asc(Chr(0))
Print Asc("A")
Print Asc("Hello")
Print Asc(Chr(200))
Print Asc(Chr(255))
Print Asc(Chr(256))
Print Asc(Chr(-1))

; Das typische Muster: Zeichen fuer Zeichen lesen, bis nichts mehr kommt
s$ = "ab"
n = 0
While Asc(s$) <> -1
	n = n + 1
	s$ = Mid(s$, 2)
Wend
Print n
