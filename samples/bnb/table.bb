; table.bb - the table, cameras and lights.
;
; Table coordinates: the playing field lies on y = 0, x from -FIELD_HALF to
; +FIELD_HALF, z from 0 (player side, open) to FIELD_BACK (back wall).
; tableborder.glb is modelled 10 times larger (inner walls at x = +-70,
; back wall at z = 220) and is scaled down to these units.

Const FIELD_HALF# = 7.0
Const FIELD_BACK# = 22.0

Global camMain, camBall, camTarget
Global tableBorder, tableFloor, tableMirror
Global lightMain
Global ballcamOn = False
Global ballcamSlide# = 0          ; 0 hidden .. 1 fully visible
Global shakeTime# = 0
Global orbitAngle# = 0

Function Table_Create()
	tableBorder = LoadMesh("assets/meshes/tableborder.glb")
	If Not tableBorder Then RuntimeError "Missing assets/meshes/tableborder.glb"
	ScaleEntity tableBorder, 0.1, 0.1, 0.1

	; placeholder floor until the textured model arrives
	tableFloor = CreateCube()
	ScaleEntity tableFloor, FIELD_HALF + 0.6, 0.1, FIELD_BACK / 2 + 0.4
	PositionEntity tableFloor, 0, -0.1, FIELD_BACK / 2
	EntityColor tableFloor, 28, 44, 62
	EntityShininess tableFloor, 0.8
	Table_SetMirror(optMirror)

	camTarget = CreatePivot()
	PositionEntity camTarget, 0, 0, 9.5

	camMain = CreateCamera()
	CameraRange camMain, 0.1, 400
	CameraClsColor camMain, 6, 8, 12
	CameraZoom camMain, 1.15

	camBall = CreateCamera()
	CameraRange camBall, 0.05, 100
	CameraClsColor camBall, 6, 8, 12
	CameraZoom camBall, 1.2
	HideEntity camBall

	lightMain = CreateLight(1)
	RotateEntity lightMain, 60, 25, 0
	LightColor lightMain, 255, 245, 230
	AmbientLight 70, 72, 85

	Table_GameView()
End Function

Function Table_SetMirror(on)
	optMirror = on
	If on
		If Not tableMirror Then tableMirror = CreateMirror()
		EntityAlpha tableFloor, 0.7
	Else
		If tableMirror Then FreeEntity tableMirror : tableMirror = 0
		EntityAlpha tableFloor, 1
	EndIf
End Function

; fixed camera for playing: the whole table in view, paddle at the bottom
Function Table_GameView()
	PositionEntity camMain, 0, 18.5, -7.8
	PointEntity camMain, camTarget
End Function

; slow flight around the table behind the menus
Function Table_Orbit(dt#)
	orbitAngle = orbitAngle + dt * 6
	PositionEntity camMain, Sin(orbitAngle) * 24, 13, 11 - Cos(orbitAngle) * 24
	PointEntity camMain, camTarget
End Function

Function Table_Shake(seconds#)
	If seconds > shakeTime Then shakeTime = seconds
End Function

; per frame while playing: camera shake and the ball camera
Function Table_Update(dt#, bx#, bz#, bvx#, bvz#)
	Table_GameView()
	If shakeTime > 0
		shakeTime = shakeTime - dt
		TurnEntity camMain, Rnd(-0.6, 0.6), Rnd(-0.6, 0.6), Rnd(-0.6, 0.6)
	EndIf

	If ballcamOn
		ballcamSlide = Clamp(ballcamSlide + dt * 4, 0, 1)
	Else
		ballcamSlide = Clamp(ballcamSlide - dt * 4, 0, 1)
	EndIf
	If ballcamSlide > 0
		ShowEntity camBall
		w = GraphicsWidth() / 4 : h = w * 9 / 16
		x = GraphicsWidth() - Int(ballcamSlide * (w + 16))
		y = GraphicsHeight() / 2 - h / 2
		CameraViewport camBall, x, y, w, h
		; behind the ball, looking along its path
		l# = Sqr(bvx * bvx + bvz * bvz)
		If l < 0.01 Then bvx = 0 : bvz = 1 : l = 1
		PositionEntity camBall, bx - bvx / l * 2.5, 1.3, bz - bvz / l * 2.5
		RotateEntity camBall, 15, -ATan2(bvx, bvz), 0      ; positive yaw turns left
	Else
		HideEntity camBall
	EndIf
End Function

Function Table_BallcamOff()
	ballcamOn = False : ballcamSlide = 0
	HideEntity camBall
End Function
