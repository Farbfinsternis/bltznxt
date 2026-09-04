; BUG-09 — Klammerausdruck als Argument: der Rest der Zeile darf nicht verloren gehen
Type Node
  Field val%
End Type

Function Add%(a%, b%)
  Return a + b
End Function

Dim grid(4)
grid(0) = 7

Local a.Node = New Node : a\val = 10
Local b.Node = New Node : b\val = 30
Local x% = 5
Local s$ = "ab"

; Klammergruppe ist nur der Anfang des Arguments
Print (1 + 2) * 3
Print (2 + 3) * (4 - 1)
Print ((1 + 2) * 3) - 1
Print (x) * 2
Print (x + 1) Mod 3
Print (x > 1) And (x < 10)
Print (s + "c") + "d"

; Feldzugriff auf geklammerten Ausdruck
Print (First Node)\val
Print (Last Node)\val
Print (First Node)\val + (Last Node)\val

; Echte Argumentlisten bleiben unberuehrt
Print Add(2, 3)
Print Add(2, 3) * 2
Print grid(0)
Print (7)
Print 1 + 2 * 3
If 1 = 1 Then Print (2 + 3) * 2 Else Print 0
Print (1 + 2) : Print (3 + 4) * 2
