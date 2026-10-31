; effects.bb - sparks and floating score numbers.

Const SPARK_MAX = 400

Type tSpark
	Field ent
	Field x#, y#, z#
	Field vx#, vy#, vz#
	Field life#, maxLife#
End Type

Type tFloat
	Field x#, y#, z#
	Field text$
	Field r, g, b
	Field life#
End Type

Global sparkTemplate
Global sparkCount = 0

Function Effects_Create()
	sparkTemplate = CreateSprite()
	ScaleSprite sparkTemplate, 0.08, 0.08
	EntityBlend sparkTemplate, 3            ; additive
	EntityFX sparkTemplate, 1 + 8           ; full bright, no fog
	HideEntity sparkTemplate
End Function

; n sparks at x,y,z in colour r,g,b
Function Sparks(n, x#, y#, z#, r, g, b, power# = 4)
	For i = 1 To n
		If sparkCount >= SPARK_MAX Then Return
		s.tSpark = New tSpark
		s\ent = CopyEntity(sparkTemplate)
		EntityColor s\ent, r, g, b
		s\x = x : s\y = y : s\z = z
		a# = Rnd(360) : v# = Rnd(0.3, 1) * power
		s\vx = Cos(a) * v : s\vz = Sin(a) * v : s\vy = Rnd(1, 2.5) * power * 0.6
		s\maxLife = Rnd(0.4, 0.9) : s\life = s\maxLife
		sparkCount = sparkCount + 1
	Next
End Function

Function FloatText(x#, z#, text$, r = 255, g = 230, b = 120)
	f.tFloat = New tFloat
	f\x = x : f\y = 0.8 : f\z = z
	f\text = text : f\r = r : f\g = g : f\b = b
	f\life = 1.0
End Function

Function Effects_Update(dt#)
	For s.tSpark = Each tSpark
		s\life = s\life - dt
		If s\life <= 0
			FreeEntity s\ent : Delete s : sparkCount = sparkCount - 1
		Else
			s\vy = s\vy - 12 * dt
			s\x = s\x + s\vx * dt : s\y = s\y + s\vy * dt : s\z = s\z + s\vz * dt
			If s\y < 0.05 Then s\y = 0.05 : s\vy = -s\vy * 0.4 : s\vx = s\vx * 0.7 : s\vz = s\vz * 0.7
			PositionEntity s\ent, s\x, s\y, s\z
			EntityAlpha s\ent, s\life / s\maxLife
		EndIf
	Next
	For f.tFloat = Each tFloat
		f\life = f\life - dt
		f\y = f\y + dt * 1.5
		If f\life <= 0 Then Delete f
	Next
End Function

; after RenderWorld: the floating numbers as 2D text over the 3D position
Function Effects_Draw()
	SetFont fontSmall
	For f.tFloat = Each tFloat
		CameraProject camMain, f\x, f\y, f\z
		a# = Clamp(f\life * 2, 0, 1)
		Color 0, 0, 0
		Text ProjectedX() + 2, ProjectedY() + 2, f\text, True, True
		Color f\r * a, f\g * a, f\b * a
		Text ProjectedX(), ProjectedY(), f\text, True, True
	Next
End Function

Function Effects_Clear()
	For s.tSpark = Each tSpark
		FreeEntity s\ent : Delete s
	Next
	sparkCount = 0
	Delete Each tFloat
End Function
