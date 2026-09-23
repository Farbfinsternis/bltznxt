; BUG-170 - Ein Befehl fuer Models auf einer Kamera bricht ab.
;
; EntityColor verlangt im Original ein Model (debugModel): Mesh, Sprite, MD2.
; Kamera, Licht und Pivot sind keine, die Meldung ist "Entity is not a model"
; (Debug-Modus des Originals, gemessen 2026-09-23). Vorher ignorierten wir
; das still. Die Meldung steht in .expected_stderr.

Graphics3D 320,240,0,2
cam = CreateCamera()
c = CreateCube()
EntityColor c,255,0,0
Print "wuerfel gefaerbt"
EntityColor cam,255,0,0
Print "nicht erreicht"
End
