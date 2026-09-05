; BUG-17, Positivseite: was erlaubt bleibt. Global und Const gehoeren auf die
; oberste Ebene des Hauptprogramms - dort und nur dort. Zugewiesen werden darf
; aus einem Block und aus einer Funktion heraus.

Global zaehler% = 0
Const SCHRITT% = 2

Function Hoch()
  zaehler = zaehler + 2
End Function

Hoch()
Hoch()
Print "global oben: " + zaehler
Print "const oben: " + SCHRITT

If zaehler > 0 Then
  zaehler = zaehler + SCHRITT
EndIf
Print "aus dem Block zugewiesen: " + zaehler
