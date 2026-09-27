; 3D-19 - Animate auf einer Entity ohne Animation bricht ab.
;
; bbAnimate wirft "Entity has no animation" - anders als die meisten
; Pruefungen auch im Release-Modus des Originals (RTEX ohne debug). Abfragen
; wie AnimSeq liefern dagegen still -1. Die Meldung steht in .expected_stderr.

Graphics3D 320,240,0,2
c = CreateCube()
Print "seq " + AnimSeq(c) + " an " + Animating(c)
Animate c
Print "nicht erreicht"
End
