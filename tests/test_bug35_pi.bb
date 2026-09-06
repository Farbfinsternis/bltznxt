; BUG-35: Pi ist ein reserviertes Wort, kein Bezeichner. Als Wert steht es
; ueberall, wo ein Ausdruck steht - im Hauptteil, im Argument ohne Klammern,
; in einer Funktion und in einer Verkettung.

Print Pi
Print "text: " + Pi

Local r# = Pi * 2.0
Print r

Const HALB# = 1.0
Print Pi * HALB

Function Umfang#(d#)
  Return d * Pi
End Function

Print Umfang(2.0)

If Pi > 3.0 Then Print "groesser als drei"
