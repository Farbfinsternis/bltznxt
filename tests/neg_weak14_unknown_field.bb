; WEAK-14 — unbekanntes Feld und unbekannter Typ werden gemeldet, statt erst
; als g++-Fehler gegen erzeugten C++-Code aufzutauchen.
Type V
  Field x%
End Type
Local p.V = New V
Print p\y
