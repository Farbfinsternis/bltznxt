; BUG-52: ein Element von "Dim feld.Punkt" nimmt nur Punkt-Objekte und Null
Type Punkt
  Field x
End Type
Dim feld.Punkt(3)
feld(0) = 5
