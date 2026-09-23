; BUG-170 - Ein Befehl mit einem ungueltigen Entity-Handle bricht ab.
;
; Wir pruefen immer wie der Debug-Modus des Originals (debugEntity in
; bbblitz3d.cpp) und melden "Entity does not exist". Das Original prueft das
; nur im Debug-Modus; im Release-Modus liest EntityX eines freigegebenen
; Wuerfels still dessen alten Wert (hier 5.0), und EntityX(0) stuerzt mit
; "Memory access violation" ab (gemessen 2026-09-23, build/ent20260923).
; Die Meldung steht in .expected_stderr.

Graphics3D 320,240,0,2
c = CreateCube()
PositionEntity c,5,6,7
Print "gueltig: " + EntityX(c)
FreeEntity c
Print "vor dem Zugriff"
Print "freigegeben: " + EntityX(c)
Print "nicht erreicht"
End
