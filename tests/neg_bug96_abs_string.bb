; BUG-96 - Abs und Sgn nehmen nur int und float. Das Original meldet
; "Illegal operator for type"; vorher nahmen wir den String an.
Local s$ = "-3"
Print Abs(s)
