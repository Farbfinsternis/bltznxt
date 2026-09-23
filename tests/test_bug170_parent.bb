; BUG-170 - Ein ungueltiger Parent bricht ab, 0 als "kein Entity" nicht.
;
; TFormPoint nimmt 0 als Weltraum und prueft nur Handles ungleich 0
; (`if( src ) debugEntity(src)`); ein Create-Befehl meldet einen ungueltigen
; Parent als "Parent entity does not exist" (debugParent). Gemessen im
; Debug-Modus des Originals am 2026-09-23. Die Meldung steht in
; .expected_stderr.

Graphics3D 320,240,0,2
c = CreateCube()
PositionEntity c,1,2,3
TFormPoint 0,0,0,c,0
Print "tform: " + TFormedX() + " " + TFormedY() + " " + TFormedZ()
p = CreatePivot(0)
Print "pivot ohne parent"
k = CreateCube(99)
Print "nicht erreicht"
End
