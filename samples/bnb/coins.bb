; coins.bb - coins drop from broken blocks and roll towards the player;
; the paddle has to catch them. They pay for the shop, the score does not
; change. (Placeholder disc until coin.png arrives.)

Const COIN_CHANCE = 30          ; percent for a normal block
Const COIN_CHANCE_SPECIAL = 60  ; percent for a special block
Const COIN_SPEED# = 4.5         ; units per second towards the paddle
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
		c\z = c\z - COIN_SPEED * dt
		c\spin = c\spin + 360 * dt
		caught = False
		If Abs(c\z - PADDLE_Z) < PADDLE_HD + COIN_R
			If Abs(c\x - paddleX) < paddleHW + COIN_R Then caught = True
		EndIf
		If caught
			gameCoins = gameCoins + 1
			Sfx(sndBonus)
			Sparks(8, c\x, 0.4, c\z, 255, 210, 80, 3)
			FloatText(c\x, c\z + 0.5, "+1 coin", 255, 210, 80)
			FreeEntity c\ent : Delete c
		ElseIf c\z < -1
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
