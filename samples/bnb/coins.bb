; coins.bb - broken blocks sometimes leave a coin where they stood; like the
; bonus objects of the original it stays there until a ball runs over it.
; They pay for the shop, the score does not change. (Placeholder disc until
; coin.png arrives.)

Const COIN_CHANCE = 30          ; percent for a normal block
Const COIN_CHANCE_SPECIAL = 60  ; percent for a special block
Const COIN_R# = 0.28

Type tCoin
	Field x#, z#
	Field ent
	Field spin#
End Type

Global gameCoins = 0
Global coinTemplate

Function Coins_Create()
	coinTemplate = CreateCylinder(20)
	ScaleEntity coinTemplate, COIN_R, 0.04, COIN_R
	EntityColor coinTemplate, 240, 190, 60
	EntityShininess coinTemplate, 1
	HideEntity coinTemplate
End Function

; maybe drops a coin at x, z (called when a block breaks)
Function Coin_Drop(x#, z#, special)
	chance = COIN_CHANCE
	If special Then chance = COIN_CHANCE_SPECIAL
	If Rand(100) > chance Then Return
	c.tCoin = New tCoin
	c\x = x : c\z = z
	c\ent = CopyEntity(coinTemplate)
	c\spin = Rnd(360)
	Coin_Place(c)
End Function

Function Coin_Place(c.tCoin)
	PositionEntity c\ent, c\x, COIN_R + 0.05, c\z
	RotateEntity c\ent, 0, c\spin, 90           ; standing on its edge, turning
End Function

Function Coins_Update(dt#)
	For c.tCoin = Each tCoin
		c\spin = c\spin + 180 * dt
		caught = False
		For b.tBall = Each tBall
			dx# = b\x - c\x : dz# = b\z - c\z
			If dx * dx + dz * dz < (ballR + COIN_R) * (ballR + COIN_R) Then caught = True : Exit
		Next
		If caught
			gameCoins = gameCoins + 1
			Sfx(sndBonus)
			Sparks(8, c\x, 0.4, c\z, 255, 210, 80, 3)
			FloatText(c\x, c\z + 0.5, "+1 coin", 255, 210, 80)
			FreeEntity c\ent : Delete c
		Else
			Coin_Place(c)
		EndIf
	Next
End Function

Function Coins_Clear()
	For c.tCoin = Each tCoin
		FreeEntity c\ent
	Next
	Delete Each tCoin
End Function
