; BUG-58: eine Funktion ohne End Function (gemessen 2026-09-08). Bisher wurde
; der Rest der Datei still Teil der Funktion.
Function F()
  Print 1
Print 2
