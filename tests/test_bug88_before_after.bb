; BUG-88: Before und After binden wie ein Vorzeichen (parseUniExpr in compiler/parser.cpp).
; "After p = Null" ist (After p) = Null, nicht After (p = Null).
Type Node
  Field val
End Type
Local a.Node = New Node : a\val = 1
Local b.Node = New Node : b\val = 2
Local c.Node = New Node : c\val = 3
Local n.Node

; Vergleich direkt hinter dem Operanden
If After c = Null Then Print "c ist letzter"
If Before a = Null Then Print "a ist erster"
If After a <> Null Then Print "a hat einen Nachfolger"
If After a = b Then Print "nach a kommt b"
Print Before Last Node = b

; verschachtelt und als Zuweisung
n = After After a
Print n\val
n = Before Before Last Node
Print n\val

; das Feld gehoert zum Operanden: After p\val waere (After (p\val)), deshalb ueber eine Variable
n = After b
Print n\val

; die uebliche Schleife ueber die Liste, vorwaerts und rueckwaerts
n = First Node
While n <> Null
  Print n\val
  n = After n
Wend
n = Last Node
Repeat
  Print n\val
  n = Before n
Until n = Null

; eine Bedingung mit And - bindet ebenfalls erst nach dem Operanden
If After a <> Null And Before c <> Null Then Print "Mitte vorhanden"
