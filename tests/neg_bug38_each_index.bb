; BUG-38 - was der Parser am Zaehler von "For <var> = Each <Type>" ablehnt.
; Der Tag am Zaehler und der Typ nach Each muessen denselben Type nennen;
; ForEachNode::semant vergleicht beide und meldet sonst "Type mismatch".

Type Punkt
  Field x%
End Type

Type Kreis
  Field r%
End Type

; Tag und Liste nennen verschiedene Typen
For p.Kreis = Each Punkt
  Print p\x
Next

; ein Zahl-Tag am Zaehler
For n% = Each Punkt
  Print n\x
Next

; ein Objekt-Tag an einer zaehlenden Schleife
For h.Punkt = 1 To 3
  Print h
Next
