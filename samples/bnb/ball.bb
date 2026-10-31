; ball.bb - paddle and balls. The physics runs in 2D on the table plane
; (x, z): circle against rectangles and circles, with sub-steps so a fast
; ball cannot pass through a block. 3D is only the picture.

Const BALL_R# = 0.3
Const PADDLE_Z# = 1.6       ; centre line of the paddle
Const PADDLE_HD# = 0.25     ; half depth
Const PADDLE_MAX_ANGLE# = 62
Const STEP_LEN# = 0.08      ; longest move per sub-step

Type tBall
	Field x#, z#
	Field vx#, vz#
	Field ent, shadow
	Field stuck             ; lies on the paddle until launched
	Field stuckOffset#
	Field spin#
End Type

Global paddleEnt
Global paddleX# = 0, paddleHW# = 1.2
Global ballSpeed# = 10      ; current speed of all balls
Global ballTemplate, shadowTemplate

Function Ball_Create()
	paddleEnt = CreateCube()
	EntityColor paddleEnt, 200, 40, 40
	EntityShininess paddleEnt, 1
	Paddle_SetWidth(1.2)

	ballTemplate = CreateSphere(20)
	ScaleEntity ballTemplate, BALL_R, BALL_R, BALL_R
	EntityColor ballTemplate, 235, 235, 245
	EntityShininess ballTemplate, 1
	HideEntity ballTemplate

	; blob shadow: a flat dark disc under the ball
	shadowTemplate = CreateCylinder(20)
	ScaleEntity shadowTemplate, BALL_R * 1.1, 0.005, BALL_R * 1.1
	EntityColor shadowTemplate, 0, 0, 0
	EntityAlpha shadowTemplate, 0.45
	EntityFX shadowTemplate, 1
	HideEntity shadowTemplate
End Function

Function Paddle_SetWidth(halfWidth#)
	paddleHW = halfWidth
	ScaleEntity paddleEnt, paddleHW, 0.25, PADDLE_HD
	Paddle_Move(0)
End Function

Function Paddle_Move(dx#)
	paddleX = Clamp(paddleX + dx, -FIELD_HALF + paddleHW, FIELD_HALF - paddleHW)
	PositionEntity paddleEnt, paddleX, 0.25, PADDLE_Z
End Function

Function Ball_Add.tBall(x#, z#, vx#, vz#, stuck)
	b.tBall = New tBall
	b\x = x : b\z = z : b\vx = vx : b\vz = vz : b\stuck = stuck
	b\stuckOffset = x - paddleX
	b\ent = CopyEntity(ballTemplate)
	b\shadow = CopyEntity(shadowTemplate)
	Ball_Place(b)
	Return b
End Function

; a new ball lying on the paddle
Function Ball_Serve()
	Ball_Add(paddleX, PADDLE_Z + PADDLE_HD + BALL_R + 0.02, 0, 0, True)
End Function

Function Ball_Place(b.tBall)
	PositionEntity b\ent, b\x, BALL_R, b\z
	PositionEntity b\shadow, b\x + 0.12, 0.01, b\z - 0.06
	; roll in the direction of motion
	TurnEntity b\ent, b\vz * 30 * 0.016, 0, -b\vx * 30 * 0.016
End Function

Function Balls_Count()
	n = 0
	For b.tBall = Each tBall : n = n + 1 : Next
	Return n
End Function

Function Balls_Clear()
	For b.tBall = Each tBall
		FreeEntity b\ent : FreeEntity b\shadow
	Next
	Delete Each tBall
End Function

; launch every ball lying on the paddle
Function Balls_Launch()
	For b.tBall = Each tBall
		If b\stuck
			b\stuck = False
			a# = Rnd(-20, 20)
			b\vx = Sin(a) * ballSpeed : b\vz = Cos(a) * ballSpeed
		EndIf
	Next
End Function

; random push for all moving balls ("nudge the table")
Function Balls_Nudge()
	For b.tBall = Each tBall
		If Not b\stuck
			a# = Rnd(-25, 25)
			vx# = b\vx * Cos(a) - b\vz * Sin(a)
			b\vz = b\vx * Sin(a) + b\vz * Cos(a) : b\vx = vx
			Ball_FixAngle(b)
		EndIf
	Next
End Function

; moves all balls; returns the points earned. Lost balls are removed
; (ballsLost counts them).
Global ballsLost = 0
Function Balls_Update(dt#)
	points = 0
	ballsLost = 0
	For b.tBall = Each tBall
		If b\stuck
			b\x = paddleX + b\stuckOffset
			b\z = PADDLE_Z + PADDLE_HD + BALL_R + 0.02
		Else
			; keep the speed of the ball in line with the game speed
			l# = Sqr(b\vx * b\vx + b\vz * b\vz)
			If l > 0.001 Then b\vx = b\vx / l * ballSpeed : b\vz = b\vz / l * ballSpeed
			steps = Int(Ceil(ballSpeed * dt / STEP_LEN))
			If steps < 1 Then steps = 1
			sdt# = dt / steps
			For i = 1 To steps
				points = points + Ball_Step(b, sdt)
				If b\z < -1.5 Then Exit
			Next
		EndIf
		If b\z < -1.5
			FreeEntity b\ent : FreeEntity b\shadow
			Delete b
			ballsLost = ballsLost + 1
		Else
			Ball_Place(b)
		EndIf
	Next
	Return points
End Function

Function Ball_Step(b.tBall, dt#)
	points = 0
	b\x = b\x + b\vx * dt
	b\z = b\z + b\vz * dt

	; side walls and back wall
	If b\x < -FIELD_HALF + BALL_R
		b\x = -FIELD_HALF + BALL_R : b\vx = Abs(b\vx) : Ball_Bounce(b, False)
	ElseIf b\x > FIELD_HALF - BALL_R
		b\x = FIELD_HALF - BALL_R : b\vx = -Abs(b\vx) : Ball_Bounce(b, False)
	EndIf
	If b\z > FIELD_BACK - BALL_R
		b\z = FIELD_BACK - BALL_R : b\vz = -Abs(b\vz) : Ball_Bounce(b, False)
	EndIf

	; paddle: the hit point decides the new angle
	If b\vz < 0 And b\z > PADDLE_Z - PADDLE_HD And b\z - BALL_R < PADDLE_Z + PADDLE_HD
		If b\x > paddleX - paddleHW - BALL_R And b\x < paddleX + paddleHW + BALL_R
			off# = Clamp((b\x - paddleX) / paddleHW, -1, 1)
			a# = off * PADDLE_MAX_ANGLE
			b\vx = Sin(a) * ballSpeed : b\vz = Cos(a) * ballSpeed
			b\z = PADDLE_Z + PADDLE_HD + BALL_R
			Sfx(sndPaddle)
			Sparks(3, b\x, 0.3, b\z - BALL_R, 255, 255, 255, 2)
		EndIf
	EndIf

	; blocks: at most one per step
	For k.tBlock = Each tBlock
		If Abs(b\x - k\x) < k\hx + BALL_R And Abs(b\z - k\z) < k\hz + BALL_R
			If k\kind = KIND_BUMPER
				hit = Ball_HitCircle(b, k)
			Else
				hit = Ball_HitBox(b, k)
			EndIf
			If hit
				If k\kind = KIND_BUMPER And k\code = "o"
					; bumpers kick the ball off a bit faster for a moment
					b\x = b\x + b\vx * 0.01 : b\z = b\z + b\vz * 0.01
				EndIf
				p = Block_Hit(k)
				If p Then points = points + p : FloatText(b\x, b\z, "+" + p)
				Exit
			EndIf
		EndIf
	Next
	Return points
End Function

; circle against the box of block k; reflects and pushes the ball out
Function Ball_HitBox(b.tBall, k.tBlock)
	nx# = Clamp(b\x, k\x - k\hx, k\x + k\hx)
	nz# = Clamp(b\z, k\z - k\hz, k\z + k\hz)
	dx# = b\x - nx : dz# = b\z - nz
	d2# = dx * dx + dz * dz
	If d2 >= BALL_R * BALL_R Then Return False
	If d2 < 0.000001
		; centre inside the box: leave on the side of least overlap
		ox# = k\hx - Abs(b\x - k\x) : oz# = k\hz - Abs(b\z - k\z)
		If ox < oz
			dx = Sgn(b\x - k\x) : dz = 0
		Else
			dx = 0 : dz = Sgn(b\z - k\z)
		EndIf
		If dx = 0 And dz = 0 Then dz = -1
		d# = 1
	Else
		d# = Sqr(d2)
		dx = dx / d : dz = dz / d
	EndIf
	; flat side or corner: on a side reflect one axis, on a corner the normal
	If Abs(dx) > 0.9
		b\vx = Abs(b\vx) * Sgn(dx)
		b\x = nx + dx * (BALL_R + 0.001)
	ElseIf Abs(dz) > 0.9
		b\vz = Abs(b\vz) * Sgn(dz)
		b\z = nz + dz * (BALL_R + 0.001)
	Else
		Ball_Reflect(b, dx, dz)
		b\x = nx + dx * (BALL_R + 0.001) : b\z = nz + dz * (BALL_R + 0.001)
	EndIf
	Ball_FixAngle(b)
	Return True
End Function

; circle against circle (bumpers, round obstacles)
Function Ball_HitCircle(b.tBall, k.tBlock)
	dx# = b\x - k\x : dz# = b\z - k\z
	r# = k\hx + BALL_R
	d2# = dx * dx + dz * dz
	If d2 >= r * r Then Return False
	d# = Sqr(d2)
	If d < 0.0001 Then dx = 0 : dz = -1 : d = 1
	dx = dx / d : dz = dz / d
	Ball_Reflect(b, dx, dz)
	b\x = k\x + dx * (r + 0.001) : b\z = k\z + dz * (r + 0.001)
	Ball_FixAngle(b)
	Return True
End Function

Function Ball_Reflect(b.tBall, nx#, nz#)
	dot# = b\vx * nx + b\vz * nz
	If dot < 0
		b\vx = b\vx - 2 * dot * nx
		b\vz = b\vz - 2 * dot * nz
	EndIf
End Function

Function Ball_Bounce(b.tBall, quiet)
	Ball_FixAngle(b)
	If Not quiet Then Sfx(sndWall)
End Function

; never let the ball run (almost) parallel to the paddle
Function Ball_FixAngle(b.tBall)
	l# = Sqr(b\vx * b\vx + b\vz * b\vz)
	If l < 0.001 Then b\vz = ballSpeed : b\vx = 0 : Return
	If Abs(b\vz) < l * 0.28
		If b\vz >= 0 Then s = 1 Else s = -1
		b\vz = s * l * 0.28
		b\vx = Sgn(b\vx) * Sqr(l * l - b\vz * b\vz)
	EndIf
End Function

; the main ball for the ball camera: the one closest to the back wall
Function Ball_Leader.tBall()
	best.tBall = Null
	For b.tBall = Each tBall
		If best = Null
			best = b
		ElseIf b\z > best\z
			best = b
		EndIf
	Next
	Return best
End Function
