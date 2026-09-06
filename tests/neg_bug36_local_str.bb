; BUG-36: 'Str' ist ein reserviertes Wort (toker.cpp fuehrt es neben Abs, Sgn,
; Int, Float und Pi). parseVarDecl liest dort einen IDENT - eine Variable
; dieses Namens gibt es nicht. Vorher nahmen wir sie an; ein "Abs -3" wurde
; dadurch zu "var_abs - 3", einem stillen Falschergebnis.
Local str$ = "x"
Print str
