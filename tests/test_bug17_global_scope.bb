; BUG-17, Positivseite: was weiterhin erlaubt sein muss. Global und Const auf
; der obersten Ebene des Hauptprogramms, und - als bewusste Erweiterung ueber
; Blitz3D hinaus, so in roadmap.md festgehalten - Global in einem Funktionsrumpf.

Global zaehler% = 0
Const SCHRITT% = 2

Function Hoch()
  zaehler = zaehler + 2
End Function

Hoch()
Hoch()
Print "global oben: " + zaehler
Print "const oben: " + SCHRITT

Function Setz()
  Global ausFunktion% = 3
End Function
Setz()
Print "global in Funktion: " + ausFunktion

; Eine Zuweisung an ein Global aus einem Block heraus bleibt erlaubt - nur die
; Deklaration gehoert nach oben.
If zaehler > 0 Then
  zaehler = zaehler + SCHRITT
EndIf
Print "aus dem Block zugewiesen: " + zaehler
