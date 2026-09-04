; WEAK-14 — Stelligkeit von Aufrufen benutzerdefinierter Funktionen
; (Blitz3D: "Not enough parameters" / "Too many parameters").
Function Add%(a%, b%)
  Return a + b
End Function
Print Add(1)
