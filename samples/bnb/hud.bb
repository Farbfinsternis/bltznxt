; hud.bb - the bar at the top while playing.
;
; The artwork assets/ui/hud.png is drawn as a textured quad in front of its
; own camera, so its alpha edges stay smooth and it scales to any screen.
; Without the file a placeholder is drawn in the same layout. Positions are
; measured on the template _work/textures/hud.png (3648 x 288 pixels) and
; work for any size of the file with the same proportions:
;
;   top row (blue):     level | time | score
;   bottom row (dark):  coins | name of the extra | 10 ball slots | extra icon
;
; 2D is laid out for 1280 x 720 and scaled with U().

Const HUD_TW# = 3648        ; template width
Const HUD_TH# = 288         ; template height
Const HUD_FILE$ = "assets/ui/hud.png"
Const HUD_WIDTH = 700      ; width on a 1280 wide screen

Global ui# = 1
Global fontBig, fontMid, fontSmall
Global fontHud, fontHudSmall          ; sized to the fields of the HUD
Global hudCam, hudQuad
Global hudX0, hudY0, hudScale#

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

; template pixel -> screen pixel
Function HX(px#)
	Return hudX0 + Int(px * hudScale)
End Function

Function HY(py#)
	Return hudY0 + Int(py * hudScale)
End Function

Function HUD_Create()
	w = U(HUD_WIDTH)
	hudScale = w / HUD_TW
	hudX0 = GraphicsWidth() / 2 - w / 2
	hudY0 = U(6)
	; text height: about three quarters of a field (fields are 130 template pixels high)
	face$ = "Arial Black"
	If FileType("assets/fonts/bloxnballs.ttf") = 1 Then face = "assets/fonts/bloxnballs.ttf"
	fontHud = LoadFont(face, Int(130 * hudScale * 0.78))
	fontHudSmall = LoadFont(face, Int(130 * hudScale * 0.62))
	If FileType(HUD_FILE) <> 1 Then Return
	tex = LoadTexture(HUD_FILE, 2)
	If Not tex Then Return

	; a camera far below the table that only sees the HUD; it renders
	; last (lowest order) and keeps the picture, clearing only depth
	hudCam = CreateCamera()
	PositionEntity hudCam, 0, -500, 0
	CameraClsMode hudCam, False, True
	CameraRange hudCam, 0.1, 10
	EntityOrder hudCam, -2

	; at distance 1 a camera with zoom 1 sees x from -1 to 1: this pivot
	; turns child coordinates into screen pixels (x right, y down)
	gw# = GraphicsWidth()
	overlay = CreatePivot(hudCam)
	PositionEntity overlay, -1, GraphicsHeight() / gw, 1
	ScaleEntity overlay, 2 / gw, -2 / gw, 1

	hudQuad = CreateMesh(overlay)
	s = CreateSurface(hudQuad)
	AddVertex s, 0, 0, 0, 0, 0
	AddVertex s, 1, 0, 0, 1, 0
	AddVertex s, 1, 1, 0, 1, 1
	AddVertex s, 0, 1, 0, 0, 1
	AddTriangle s, 0, 1, 2
	AddTriangle s, 0, 2, 3
	EntityTexture hudQuad, tex
	EntityFX hudQuad, 1 + 8 + 16            ; full bright, no fog, both sides
	PositionEntity hudQuad, hudX0, hudY0, 0
	ScaleEntity hudQuad, w, w * HUD_TH / HUD_TW, 1
	HUD_Show(False)
End Function

Function HUD_Show(on)
	If Not hudCam Then Return
	If on Then ShowEntity hudCam Else HideEntity hudCam
End Function

; placeholder in the layout of the artwork
Function HUD_DrawPlaceholder()
	Color 70, 74, 82 : Rect HX(0), HY(0), HX(HUD_TW) - HX(0), HY(HUD_TH) - HY(0), True
	Color 26, 62, 120
	Rect HX(600), HY(5), HX(998) - HX(600), HY(135) - HY(5), True
	Rect HX(1020), HY(5), HX(1640) - HX(1020), HY(135) - HY(5), True
	Rect HX(1660), HY(5), HX(3080) - HX(1660), HY(135) - HY(5), True
	Color 34, 36, 40
	Rect HX(110), HY(152), HX(980) - HX(110), HY(285) - HY(152), True
	Rect HX(1025), HY(152), HX(1615) - HX(1025), HY(285) - HY(152), True
	Rect HX(3172), HY(152), HX(3557) - HX(3172), HY(285) - HY(152), True
	For i = 0 To 9
		cx = HX(1782 + i * 137.6) : cy = HY(215) : r = HY(60) - HY(0)
		Oval cx - r, cy - r, 2 * r, 2 * r, True
	Next
End Function

Function HUD_Draw()
	If Not hudCam Then HUD_DrawPlaceholder()

	; top row: level | time | score
	SetFont fontHud
	ty = HY(70)
	Color 255, 255, 255
	Text HX(740), ty, Right("00" + gameLevel, 2), True, True
	t = Int(Ceil(timeLeft))
	If t < 0 Then t = 0
	If t <= 15 And (MilliSecs() / 300) Mod 2 Then Color 255, 90, 70
	Text HX(1280), ty, (t / 60) + ":" + Right("0" + (t Mod 60), 2), True, True
	Color 255, 255, 255
	s$ = Right("000000" + gameScore, 7)
	Text HX(2960) - StringWidth(s), ty, s, False, True

	; bottom row: coins | extra name | balls | extra icon
	SetFont fontHudSmall
	by = HY(218)
	Color 230, 190, 70 : Text HX(545), by, "0", True, True
	; selected item of the inventory; the icon field shows its place until
	; the icons arrive, red while an atomic ball is armed
	If invCount > 0
		Color 230, 235, 245 : Text HX(1320), by, Inv_Current(), True, True
		Color 160, 165, 175 : Text HX(3364), by, (invSel + 1) + "/" + invCount, True, True
	Else
		Color 150, 150, 160 : Text HX(1320), by, "-", True, True
		Text HX(3364), by, "-", True, True
	EndIf
	If atomicArmed
		Color 255, 90, 60 : Text HX(3364), by, "ARMED", True, True
	EndIf
	; slots show the balls in reserve (the one in play is not counted)
	reserve = gameLives - 1
	r = HY(44) - HY(0)
	For i = 0 To 9
		If i < reserve
			cx = HX(1782 + i * 137.6) : cy = HY(215)
			If i = 9 And reserve > 10
				Color 255, 255, 255 : Text cx, cy, "+" + (reserve - 9), True, True
			Else
				Color 230, 230, 240 : Oval cx - r, cy - r, 2 * r, 2 * r, True
			EndIf
		EndIf
	Next

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
