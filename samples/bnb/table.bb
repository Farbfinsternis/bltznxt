; table.bb - the table, cameras and lights.
;
; Table coordinates: the playing field lies on y = 0, x from -FIELD_HALF to
; +FIELD_HALF, z from 0 (player side, open) to FIELD_BACK (back wall).
; tableborder.glb is modelled 10 times larger (inner walls at x = +-70,
; back wall at z = 220) and is scaled down to these units.

Const FIELD_HALF# = 7.0
Const FIELD_BACK# = 22.0

Global camMain, camBall, camTarget, camLook
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
	camLook = CreatePivot()

	camMain = CreateCamera()
	CameraRange camMain, 0.1, 400
	CameraClsColor camMain, 6, 8, 12
	CameraZoom camMain, 1.0

	camBall = CreateCamera()
	CameraRange camBall, 0.05, 100
	CameraClsColor camBall, 6, 8, 12
	CameraZoom camBall, 1.2
	EntityOrder camBall, -1                 ; over the main picture, under the HUD
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

; Camera for playing, like the original: low and close to the table, it
; follows the ball. Two poses, blended by camFollow (0 = ball at the
; paddle, 1 = ball at the back wall):
;   front: low and close, the table is big, the back rows may be cut off
;   back:  higher and further back, looking deeper so the ball stays in view
; camFollow and the sideways shift glide towards their targets.
Const CAM_FRONT_Y# = 6.0, CAM_FRONT_Z# = -2.5, CAM_FRONT_LOOK# = 7.5
Const CAM_BACK_Y# = 9.0, CAM_BACK_Z# = -4.0, CAM_BACK_LOOK# = 11.0
Const CAM_GLIDE# = 2.5      ; how fast the camera follows (per second)
Const CAM_SIDE# = 0.25      ; share of the ball's x the camera moves along

Global camFollow# = 0, camSideX# = 0

Function Table_FollowTarget#(bz#)
	t# = Clamp((bz - 4) / 14, 0, 1)
	Return t * t * (3 - 2 * t)                  ; smoothstep: soft at both ends
End Function

; places the play camera for the current camFollow / camSideX
Function Table_GameView()
	f# = camFollow
	y# = CAM_FRONT_Y + (CAM_BACK_Y - CAM_FRONT_Y) * f
	z# = CAM_FRONT_Z + (CAM_BACK_Z - CAM_FRONT_Z) * f
	look# = CAM_FRONT_LOOK + (CAM_BACK_LOOK - CAM_FRONT_LOOK) * f
	PositionEntity camMain, camSideX, y, z
	PositionEntity camLook, camSideX * 1.4, 0, look
	PointEntity camMain, camLook
End Function

; jumps straight to the pose for a ball at bx, bz (level start, resume)
Function Table_SnapView(bx#, bz#)
	camFollow = Table_FollowTarget(bz)
	camSideX = bx * CAM_SIDE
	Table_GameView()
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
	k# = 1 - Exp(-CAM_GLIDE * dt)               ; same glide at any frame rate
	camFollow = camFollow + (Table_FollowTarget(bz) - camFollow) * k
	camSideX = camSideX + (bx * CAM_SIDE - camSideX) * k
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
