; BUG-29: Die Auswertungsreihenfolge steht jetzt im erzeugten Code, nicht mehr
; beim C++-Compiler. Blitz3D selbst legt keine fest (Tile::eval entscheidet nach
; Registerbedarf) - wir sagen bewusst mehr zu: von links nach rechts.

Global a% = 7
Dim feld%(2)

Function G%()
  a = 4
  Return 1
End Function

Function H%()
  feld(0) = 9
  Return 1
End Function

Function Rein%(x%)
  Return x * 2
End Function

; Das Global wird NACH dem Aufruf gelesen: 4, nicht 7.
Print "x" + G() + " " + a

; Und hier davor: 7 + 1, nicht 4 + 1.
a = 7
Print a + G()

; Beides in einem Ausdruck: davor 7, danach 4.
a = 7
Local n% = a + G() + a
Print n

; Dasselbe fuer ein Array-Element.
feld(0) = 5
Print feld(0) + H()
Print feld(0)

; Eine reine Funktion aendert nichts, der Ausdruck bleibt wie er war.
a = 3
Print Rein(2) + a
