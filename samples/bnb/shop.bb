; shop.bb - between two levels the coins buy items for the inventory.
; (Placeholder screen until shop.png and the item icons arrive.)

Const WARES = 8
Dim wareName$(WARES - 1)
Dim warePrice(WARES - 1)

Function Shop_Init()
	Restore ShopWares
	For i = 0 To WARES - 1
		Read wareName(i), warePrice(i)
	Next
End Function

.ShopWares
Data "Extra ball", 12
Data "Atomic ball", 8
Data "Multiball", 6
Data "Big ball", 6
Data "Score x2", 5
Data "Wide paddle", 5
Data "Slow motion", 4
Data "Time +15", 3

Function Shop_Open()
	Sfx(sndShop)
	menuSel = 0
	SetState(ST_SHOP)
End Function

Function Shop_Update(dt#)
	Table_Orbit(dt)
	Effects_Update(dt)
	If autoPlay
		; tests: buy the dearest item we can afford, then go on
		If stateTime > 1 And stateTime - dt <= 1
			For i = 0 To WARES - 1
				If warePrice(i) <= gameCoins Then Shop_Buy(i) : Exit
			Next
		EndIf
		If stateTime > 2 Then Shop_Leave()
	EndIf
End Function

Function Shop_Draw()
	cx = GraphicsWidth() / 2
	; placeholder panel behind the list
	Color 20, 22, 26 : Rect cx - U(380), U(70), U(760), U(600), True
	Color 90, 95, 105 : Rect cx - U(380), U(70), U(760), U(600), False

	SetFont fontBig : Color 240, 190, 60
	Text cx, U(120), "SHOP", True, True
	SetFont fontSmall : Color 230, 190, 70
	Text cx, U(175), "Coins: " + gameCoins + "      Items: " + invCount + " / " + INV_MAX, True, True

	menuCount = 0
	For i = 0 To WARES - 1
		Menu_Add(100 + i, wareName(i) + "   -   " + warePrice(i) + " coins")
	Next
	Menu_Add(99, "Next level  >")
	If menuSel >= menuCount Then menuSel = 0
	id = Menu_Run(U(230))
	If id = 99
		If menuStep = 0 Then Shop_Leave()
	ElseIf id >= 100 And menuStep = 0
		Shop_Buy(id - 100)
	EndIf
	If KeyHit(1) Then Shop_Leave()
	Message_Draw()
End Function

Function Shop_Buy(i)
	If gameCoins < warePrice(i)
		Message("Not enough coins", 2)
		Return
	EndIf
	If invCount >= INV_MAX
		Message("The inventory is full", 2)
		Return
	EndIf
	gameCoins = gameCoins - warePrice(i)
	Inv_Add(wareName(i))
	Sfx(sndBonus)
	Message(wareName(i) + " bought", 2)
End Function

Function Shop_Leave()
	Game_StartLevel(gameLevel + 1)
End Function
