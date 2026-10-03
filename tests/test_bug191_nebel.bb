; BUG-191 - Kameranebel: CameraFogMode, CameraFogRange, CameraFogColor.
;
; Am Original gemessen (2026-10-03), Wuerfel mit EntityFX 1, Vorderseite
; senkrecht zur Blickachse, Hintergrund blau. Nebel ist linear zwischen near
; und far; der Abstand ist die Tiefe entlang der Blickachse, der Bildrand
; bekommt also genau so viel Nebel wie die Mitte. Modus 2 und EntityFX 8
; zeichnen ohne Nebel. Vorgabe: schwarz, 1 bis 1000.
;
; Bewusst anders als das Original (gxScene mischt jede Flaeche zur
; Nebelfarbe): additiv gemischte Flaechen verblassen nach Schwarz,
; multiplizierte nach Weiss. Und eine Parallelprojektion bekommt Nebel nach
; der echten Tiefe; das Original zeigt dort keinen.

Graphics3D 320,240,0,2
cam = CreateCamera() : CameraClsColor cam,0,0,255
c = CreateCube() : EntityFX c,1 : ScaleEntity c,100,100,1

Function Px$(name$)
	RenderWorld
	Print name + " " + Hex(ReadPixel(160,120) And $FFFFFF) + " " + Hex(ReadPixel(3,120) And $FFFFFF)
End Function

PositionEntity c,0,0,16
Px("ohne nebel")
CameraFogMode cam,1
Px("vorgabe z15")
CameraFogRange cam,10,20 : CameraFogColor cam,255,0,0
Px("10-20 rot z15")
PositionEntity c,0,0,13 : Px("z12")
PositionEntity c,0,0,26 : Px("z25")
PositionEntity c,0,0,6  : Px("z5")
PositionEntity c,0,0,16
CameraFogRange cam,20,10 : Px("umgekehrt z15")
CameraFogRange cam,10,20
CameraFogMode cam,2 : Px("modus 2")
CameraFogMode cam,1
EntityFX c,1+8 : Px("fx 8")
EntityFX c,1
EntityAlpha c,0.5 : CameraClsColor cam,0,0,0 : Px("alpha 0.5")
EntityAlpha c,1
EntityBlend c,3 : Px("add verblasst")
EntityBlend c,2 : CameraClsColor cam,0,0,128 : Px("multiply verblasst")
EntityBlend c,1
CameraProjMode cam,2 : CameraZoom cam,0.01 : Px("ortho z15")
CameraProjMode cam,1 : CameraZoom cam,1

; Zweite Kamera ohne Nebel: der Nebel gehoert der Kamera.
CameraViewport cam,0,0,160,240
cam2 = CreateCamera() : CameraViewport cam2,160,0,160,240 : CameraClsColor cam2,0,0,255
RenderWorld
Print "je kamera " + Hex(ReadPixel(80,120) And $FFFFFF) + " " + Hex(ReadPixel(240,120) And $FFFFFF)
End
