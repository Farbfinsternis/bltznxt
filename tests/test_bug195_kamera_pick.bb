; BUG-195 (Gruppe 1) - CameraPick, CameraProject, ProjectedX/Y/Z, EntityInView.
;
; Alle Werte am Original gemessen (2026-10-03), nach bbblitz3d.cpp des
; Originals umgesetzt. Dabei gefunden und mitgeprueft:
; - nach einem Fehltreffer behalten PickedX/Y/Z und PickedNX/NY/NZ den
;   vorigen Treffer, nur PickedTime wird 1 (World::traceRay);
; - ProjectedZ ist immer die nahe Ebene;
; - die Parallelprojektion ist near*2/zoom breit: mit near 0.5 statt 1 ist
;   der Wuerfel bei x=7 nicht mehr im Bild (gxScene::setOrthoProj).
Graphics3D 640, 480, 32, 2
SetBuffer BackBuffer()

Global cam = CreateCamera()
Global wuerfel = CreateCube() : EntityPickMode wuerfel, 2 : EntityFX wuerfel, 1 : EntityColor wuerfel, 255, 0, 0
PositionEntity wuerfel, 0, 0, 10
Global wand = CreateCube() : EntityPickMode wand, 2 : ScaleEntity wand, 50, 50, 1 : PositionEntity wand, 0, 0, 40

Function F$(x#)
	Return Int(Floor(x * 1000 + 0.5))
End Function
Function Pick(name$, x#, y#)
	e = CameraPick(cam, x, y)
	n$ = "nichts"
	If e = wuerfel Then n = "wuerfel"
	If e = wand Then n = "wand"
	Print name + ": " + n + " p=" + F(PickedX()) + "," + F(PickedY()) + "," + F(PickedZ()) + " n=" + F(PickedNX()) + "," + F(PickedNY()) + "," + F(PickedNZ()) + " t=" + F(PickedTime())
End Function
Function Proj(name$, x#, y#, z#)
	CameraProject cam, x, y, z
	Print name + ": " + F(ProjectedX()) + "," + F(ProjectedY()) + "," + F(ProjectedZ())
End Function

Pick("mitte", 320, 240)
Pick("links oben", 100, 50)
Pick("rand", 0, 0)
Pick("ausserhalb", -50, 700)
CameraZoom cam, 2 : Pick("zoom2", 400, 300) : CameraZoom cam, 1
PositionEntity cam, 5, 2, -3 : TurnEntity cam, 10, -20, 5
Pick("gedreht", 320, 240) : Pick("gedreht ecke", 600, 400)
Proj("proj gedreht", 1, 1, 10)
PositionEntity cam, 0, 0, 0 : RotateEntity cam, 0, 0, 0
CameraRange cam, 0.5, 30
Pick("range 0.5-30 wand weg", 320, 240)
Pick("range wuerfel", 330, 250)
CameraRange cam, 1, 1000
CameraViewport cam, 100, 50, 320, 240
Pick("viewport mitte", 160, 120) : Proj("viewport proj", 0, 0, 10)
CameraViewport cam, 0, 0, 640, 480
Proj("proj mitte", 0, 0, 10)
Proj("proj versetzt", 3, -2, 10)
Proj("proj hinten", 0, 0, -5)
Proj("proj zu fern", 0, 0, 2000)
Proj("proj nah", 0, 0, 0.5)
CameraProjMode cam, 2 : CameraZoom cam, 0.1
Pick("ortho", 600, 240)
Proj("ortho proj", 7, 3, 10)
CameraRange cam, 0.5, 1000
Pick("ortho near0.5", 600, 240)
Proj("ortho near0.5 proj", 7, 3, 10)
CameraRange cam, 1, 1000 : CameraZoom cam, 1 : CameraProjMode cam, 1

; EntityInView
piv = CreatePivot() : PositionEntity piv, 0, 0, 5
spr = CreateSprite() : PositionEntity spr, 0, 0, -5
Print "inview vorn " + EntityInView(wuerfel, cam) + " pivot " + EntityInView(piv, cam) + " sprite hinten " + EntityInView(spr, cam)
PositionEntity wuerfel, 11.5, 0, 10 : Print "inview teilweise " + EntityInView(wuerfel, cam)
PositionEntity wuerfel, 12.5, 0, 10 : Print "inview knapp daneben " + EntityInView(wuerfel, cam)
PositionEntity wuerfel, 0, 0, 10 : CameraRange cam, 1, 5 : Print "inview hinter range " + EntityInView(wuerfel, cam)
CameraRange cam, 1, 1000 : HideEntity wuerfel : Print "inview versteckt " + EntityInView(wuerfel, cam) : ShowEntity wuerfel
ScaleEntity wuerfel, 3, 3, 3 : PositionEntity wuerfel, 14, 0, 10 : Print "inview skaliert " + EntityInView(wuerfel, cam)
ScaleEntity wuerfel, 1, 1, 1 : PositionEntity wuerfel, 0, 0, 10

; Ortho-Darstellung: Wuerfel bei x=7, zoom 0.1; near 1 gegen near 0.5
HideEntity wand
CameraProjMode cam, 2 : CameraZoom cam, 0.1 : PositionEntity wuerfel, 7, 0, 10
CameraClsColor cam, 0, 0, 255
For nr = 1 To 2
	If nr = 1 Then CameraRange cam, 1, 1000 Else CameraRange cam, 0.5, 1000
	RenderWorld
	LockBuffer BackBuffer() : c = ReadPixelFast(544, 240) And $FFFFFF : c2 = ReadPixelFast(320, 240) And $FFFFFF : UnlockBuffer BackBuffer()
	Print "ortho bild near " + nr + ": 544=" + Right(Hex(c), 6) + " 320=" + Right(Hex(c2), 6)
Next
End
