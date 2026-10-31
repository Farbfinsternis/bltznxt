; hud.bb - the bar at the top while playing (placeholder in the layout of
; _work/textures/hud.png: score | level | time on top, ten ball slots below,
; the selected extra on the right, coins bottom left).
; 2D is laid out for 1280 x 720 and scaled with U().

Global ui# = 1
Global fontBig, fontMid, fontSmall

Function U(v#)
	Return Int(v * ui)
End Function

Function Fonts_Load()
	ui = GraphicsWidth() / 1280.0
	face$ = "Arial Black"
	If FileType("assets/fonts/bloxnballs.ttf") = 1 Then face = "assets/fonts/bloxnballs.ttf"
	fontBig = LoadFont(face, U(56))
	fontMid = LoadFont(face, U(30))
	fontSmall = LoadFont(face, U(20))
End Function

Function HUD_Draw()
	x0 = GraphicsWidth() / 2 - U(500) : y0 = U(8)
	w = U(1000) : h = U(78)

	; frame
	Color 60, 64, 72 : Rect x0, y0, w, h, True
	Color 120, 126, 136 : Rect x0, y0, w, h, False
	; blue upper half: score | level | time
	Color 26, 62, 120
	Rect x0 + U(60), y0 + U(4), U(260), U(34), True
	Rect x0 + U(330), y0 + U(4), U(140), U(34), True
	Rect x0 + U(480), y0 + U(4), U(380), U(34), True

	SetFont fontMid
	Color 255, 255, 255
	Text x0 + U(310) - StringWidth(Right("000000" + gameScore, 7)), y0 + U(21), Right("000000" + gameScore, 7), False, True
	Text x0 + U(400), y0 + U(21), Right("00" + gameLevel, 2), True, True
	t = Int(Ceil(timeLeft))
	If t < 0 Then t = 0
	If t <= 15 And (MilliSecs() / 300) Mod 2 Then Color 255, 90, 70
	Text x0 + U(670), y0 + U(21), (t / 60) + ":" + Right("0" + (t Mod 60), 2), True, True

	; lower half: coins | ten ball slots | extra
	SetFont fontSmall
	Color 30, 32, 36 : Rect x0 + U(60), y0 + U(42), U(160), U(32), True
	Color 200, 170, 60 : Text x0 + U(140), y0 + U(58), "0", True, True
	For i = 0 To 9
		cx = x0 + U(250) + i * U(36) : cy = y0 + U(58)
		Color 30, 32, 36 : Oval cx - U(14), cy - U(14), U(28), U(28), True
		; slots show the balls in reserve (the one in play is not counted)
		If i < gameLives - 1
			Color 230, 230, 240 : Oval cx - U(10), cy - U(10), U(20), U(20), True
		EndIf
	Next
	Color 30, 32, 36 : Rect x0 + U(880), y0 + U(8), U(110), U(64), True
	Color 140, 140, 150 : Text x0 + U(935), y0 + U(40), "-", True, True

	; centre message (level name, time out)
	If msgTime > 0
		SetFont fontBig
		a# = Clamp(msgTime, 0, 1)
		Color 0, 0, 0 : Text GraphicsWidth() / 2 + U(3), GraphicsHeight() / 2 + U(3), msgText, True, True
		Color 255 * a, 220 * a, 120 * a : Text GraphicsWidth() / 2, GraphicsHeight() / 2, msgText, True, True
	EndIf

	; hint while the ball waits on the paddle
	For b.tBall = Each tBall
		If b\stuck
			SetFont fontSmall : Color 200, 200, 210
			Text GraphicsWidth() / 2, GraphicsHeight() - U(30), "Click to launch", True, True
			Exit
		EndIf
	Next
End Function
