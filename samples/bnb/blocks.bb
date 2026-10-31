; blocks.bb - the blocks of a level: building, drawing state and hits.

Const KIND_NORMAL  = 1      ; 1..3 hits
Const KIND_SPECIAL = 2      ; one hit, carries an extra (points only until the extras arrive)
Const KIND_BUMPER  = 3      ; indestructible, kicks the ball
Const KIND_WALL    = 4      ; indestructible obstacle

Const BLOCK_TOP# = 21.5     ; z of the back edge of the first row

Type tBlock
	Field x#, z#            ; centre on the table
	Field hx#, hz#          ; half size (for bumpers: hx = radius)
	Field code$
	Field kind
	Field hits, maxHits
	Field ent
	Field flash#            ; > 0 right after a hit
End Type

Global blocksLeft = 0       ; destructible blocks still standing
Global cellSize# = 1
Global gridLeft# = 0         ; x of the left edge of the grid

Function Blocks_Build()
	Blocks_Clear()
	cellSize = 2 * FIELD_HALF / lvCols
	If lvRows * cellSize > 15 Then cellSize = 15.0 / lvRows
	gridLeft = -lvCols * cellSize / 2
	For r = 0 To lvRows - 1
		For c = 0 To lvCols - 1
			code$ = Level_Cell(c, r)
			If code <> "."
				Block_Create(code, gridLeft + (c + 0.5) * cellSize, BLOCK_TOP - (r + 0.5) * cellSize)
			EndIf
		Next
	Next
End Function

Function Block_Create(code$, x#, z#)
	b.tBlock = New tBlock
	b\code = code : b\x = x : b\z = z
	b\hx = cellSize * 0.46 : b\hz = cellSize * 0.46
	h# = cellSize * 0.32
	Select code
	Case "1", "2", "3"
		b\kind = KIND_NORMAL : b\maxHits = Int(code)
	Case "o"
		b\kind = KIND_BUMPER : b\maxHits = 0
	Case "|", "#", "/"
		b\kind = KIND_WALL : b\maxHits = 0
	Default
		b\kind = KIND_SPECIAL : b\maxHits = 1
	End Select
	b\hits = b\maxHits

	Select b\kind
	Case KIND_BUMPER
		b\hx = cellSize * 0.38 : b\hz = b\hx
		b\ent = CreateCylinder(20)
		ScaleEntity b\ent, b\hx, h * 1.4, b\hx
		PositionEntity b\ent, x, h * 1.4, z
		EntityColor b\ent, 200, 210, 230
		EntityShininess b\ent, 1
	Case KIND_WALL
		If code = "|"
			b\hx = cellSize * 0.3 : b\hz = b\hx
			b\ent = CreateCylinder(16)
		Else
			b\ent = CreateCube()
		EndIf
		ScaleEntity b\ent, b\hx, h * 1.2, b\hz
		PositionEntity b\ent, x, h * 1.2, z
		EntityColor b\ent, 70, 74, 82
		EntityShininess b\ent, 0.5
		If code = "|" Then b\kind = KIND_BUMPER : b\code = "|"
	Default
		b\ent = CreateCube()
		ScaleEntity b\ent, b\hx, h, b\hz
		PositionEntity b\ent, x, h, z
		EntityShininess b\ent, 0.4
		blocksLeft = blocksLeft + 1
	End Select
	Block_Paint(b)
End Function

; colour by type and damage (placeholder until the block atlas arrives)
Function Block_Paint(b.tBlock)
	r = 0 : g = 0 : bl = 0
	Select b\kind
	Case KIND_NORMAL
		Select b\maxHits
		Case 1 : r = 70  : g = 140 : bl = 220
		Case 2 : r = 230 : g = 140 : bl = 40
		Default : r = 210 : g = 50 : bl = 50
		End Select
		; damaged blocks get darker ("cracks" come with the textures)
		f# = 0.45 + 0.55 * Float(b\hits) / b\maxHits
		r = r * f : g = g * f : bl = bl * f
	Case KIND_SPECIAL
		r = 240 : g = 200 : bl = 60
	Default
		Return
	End Select
	If b\flash > 0
		r = r + (255 - r) * b\flash : g = g + (255 - g) * b\flash : bl = bl + (255 - bl) * b\flash
	EndIf
	EntityColor b\ent, r, g, bl
End Function

Function Blocks_Update(dt#)
	For b.tBlock = Each tBlock
		If b\flash > 0
			b\flash = b\flash - dt * 5
			If b\flash < 0 Then b\flash = 0
			If b\kind = KIND_BUMPER And b\code = "o"
				s# = 1 + b\flash * 0.25
				ScaleEntity b\ent, b\hx * s, cellSize * 0.45, b\hx * s
			Else
				Block_Paint(b)
			EndIf
		EndIf
	Next
End Function

; the ball hit block b; returns the points earned. smash = break it with
; this hit, however many it would still take (big ball)
Function Block_Hit(b.tBlock, smash = False)
	b\flash = 1
	Select b\kind
	Case KIND_BUMPER
		If b\code = "o"
			Sfx(sndBumper)
			Sparks(4, b\x, 0.5, b\z, 120, 170, 255, 3)
			Return 1
		EndIf
		Sfx(sndWall)
		Return 0
	Case KIND_WALL
		Sfx(sndWall)
		Return 0
	End Select

	b\hits = b\hits - 1
	If smash Then b\hits = 0
	If b\hits > 0
		Sfx(sndBlockHit)
		Block_Paint(b)
		Sparks(3, b\x, 0.5, b\z, 255, 200, 120, 2)
		Return 10
	EndIf

	; destroyed
	points = 15 * b\maxHits
	r = 255 : g = 160 : bl = 80
	If b\kind = KIND_SPECIAL
		Select b\code
		Case "P" : points = 50
		Case "Q" : points = 75
		Case "R" : points = 100
		Default  : points = 25
		End Select
		Sfx(sndBonus)
		r = 255 : g = 230 : bl = 90
	Else
		Sfx(sndBlock)
	EndIf
	Sparks(14, b\x, 0.5, b\z, r, g, bl, 4)
	x# = b\x : z# = b\z : code$ = b\code : kind = b\kind
	FreeEntity b\ent
	Delete b
	blocksLeft = blocksLeft - 1
	Coin_Drop(x, z, kind = KIND_SPECIAL)
	If kind = KIND_SPECIAL
		label$ = Extra_Trigger(code, x, z)
		If label <> "" Then FloatText(x, z + 0.6, label, 140, 220, 255)
	EndIf
	Return points
End Function

; atomic ball: destroys every breakable block within radius r of x, z
Function Blocks_Explode(x#, z#, r#)
	points = 0
	For b.tBlock = Each tBlock
		If b\kind = KIND_NORMAL Or b\kind = KIND_SPECIAL
			dx# = b\x - x : dz# = b\z - z
			If dx * dx + dz * dz <= r * r
				points = points + 15 * b\maxHits
				Sparks(10, b\x, 0.5, b\z, 255, 120, 40, 6)
				Coin_Drop(b\x, b\z, False)
				FreeEntity b\ent
				Delete b
				blocksLeft = blocksLeft - 1
			EndIf
		EndIf
	Next
	Sfx(sndBomb)
	Table_Shake(0.4)
	FloatText(x, z + 0.6, "Boom!", 255, 140, 60)
	Return points
End Function

; new blocks: fills the empty cells in the 5 x 5 cells around x, z with
; one-hit blocks; returns how many
Function Blocks_Respawn(x#, z#)
	c0 = Floor((x - gridLeft) / cellSize) : r0 = Floor((BLOCK_TOP - z) / cellSize)
	n = 0
	For r = r0 - 2 To r0 + 2
		For c = c0 - 2 To c0 + 2
			If c >= 0 And c < lvCols And r >= 0 And r < lvRows
				cx# = gridLeft + (c + 0.5) * cellSize
				cz# = BLOCK_TOP - (r + 0.5) * cellSize
				free = True
				For b.tBlock = Each tBlock
					If Abs(b\x - cx) < cellSize * 0.5 And Abs(b\z - cz) < cellSize * 0.5 Then free = False : Exit
				Next
				; never on top of a ball
				For ball.tBall = Each tBall
					If Abs(ball\x - cx) < cellSize * 0.5 + ballR And Abs(ball\z - cz) < cellSize * 0.5 + ballR Then free = False
				Next
				If free
					Block_Create("1", cx, cz)
					Sparks(3, cx, 0.5, cz, 120, 200, 255, 2)
					n = n + 1
				EndIf
			EndIf
		Next
	Next
	Return n
End Function

Function Blocks_Clear()
	For b.tBlock = Each tBlock
		FreeEntity b\ent
	Next
	Delete Each tBlock
	blocksLeft = 0
End Function

Function Blocks_Show(on)
	For b.tBlock = Each tBlock
		If on Then ShowEntity b\ent Else HideEntity b\ent
	Next
End Function
