; BUG-38 - was der semantische Pass am Zaehler von "For <var> = Each <Type>"
; ablehnt, wenn der Zaehler ohne Tag geschrieben ist und schon existiert.
; ForEachNode::semant loest ihn wie jede andere Variable auf und verlangt
; dann ein Objekt genau dieses Typs ("Index variable is not a NewType",
; "Type mismatch").

Type Punkt
  Field x%
End Type

Type Kreis
  Field r%
End Type

; der Zaehler ist schon eine Zahl
Local k% = 0
For k = Each Punkt
  Print k\x
Next

; der Zaehler ist schon ein Objekt, aber vom anderen Typ
Local c.Kreis = New Kreis
For c = Each Punkt
  Print c\x
Next
