; BUG-35: In Blitz3D ist Pi ein reserviertes Wort (toker.cpp:
; alphaTokes["Pi"]=PI), parseVarDecl liest dort einen IDENT. Eine Konstante
; dieses Namens gibt es deshalb nicht - vorher nahmen wir sie an und ersetzten
; jede Benutzung stillschweigend durch das Builtin.
Const Pi# = 3.5
Print Pi
