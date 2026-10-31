; menu.bb - main menu, options, high scores, credits, name entry and the
; screens between levels. Mouse, keyboard and gamepad all work.

Const MENU_MAX = 12
Dim menuText$(MENU_MAX - 1)
Dim menuId(MENU_MAX - 1)
Global menuCount = 0
Global menuSel = 0
Global menuSkill = 1
Global scoreView = 1
Global nameText$ = ""
Global nameRank = -1
Global menuLastMouseY = -1
Global joyLastX = 0, joyLastY = 0

Const M_RESUME = 1, M_CONTINUE = 2, M_NEW = 3, M_SKILL = 4, M_SCORES = 5
Const M_OPTIONS = 6, M_CREDITS = 7, M_QUIT = 8
Const M_MUSIC = 20, M_SOUND = 21, M_MOUSE = 22, M_MIRROR = 23
Const M_SIZE = 24, M_FULL = 25, M_BACK = 26

Function Menu_Add(id, t$)
	menuText(menuCount) = t : menuId(menuCount) = id
	menuCount = menuCount + 1
End Function

Function Menu_BuildMain()
	menuCount = 0
	If gameActive
		Menu_Add(M_RESUME, "Resume")
	ElseIf saveValid
		Menu_Add(M_CONTINUE, "Continue  (level " + saveLevel + ", " + skillName(saveSkill) + ")")
	EndIf
	Menu_Add(M_NEW, "New Game")
	Menu_Add(M_SKILL, "Skill:  " + skillName(menuSkill))
	Menu_Add(M_SCORES, "Highscores")
	Menu_Add(M_OPTIONS, "Options")
	Menu_Add(M_CREDITS, "Credits")
	Menu_Add(M_QUIT, "Quit")
	If menuSel >= menuCount Then menuSel = 0
End Function

Function Menu_BuildOptions()
	menuCount = 0
	Menu_Add(M_MUSIC, "Music:  " + Int(optMusic * 100 + 0.5) + "%")
	Menu_Add(M_SOUND, "Sound:  " + Int(optSound * 100 + 0.5) + "%")
	Menu_Add(M_MOUSE, "Mouse speed:  " + Int(optMouse * 100 + 0.5) + "%")
	If optMirror Then t$ = "On" Else t = "Off"
	Menu_Add(M_MIRROR, "Reflections:  " + t)
	Menu_Add(M_SIZE, "Window:  " + optWidth + " x " + optHeight)
	If optFullscreen Then t = "On" Else t = "Off"
	Menu_Add(M_FULL, "Fullscreen:  " + t)
	Menu_Add(M_BACK, "Back")
	If menuSel >= menuCount Then menuSel = 0
End Function

; draws the list centred at y0; returns the id activated (click / Enter),
; and sets menuStep to -1 / +1 for Left / Right on the selected entry
Global menuStep = 0
Function Menu_Run(y0)
	menuStep = 0
	lineH = U(52)
	; the mouse selects the row under it, but only when it moves
	; (so keys and gamepad are not overruled by a resting pointer)
	my = MouseY()
	If my <> menuLastMouseY
		menuLastMouseY = my
		For i = 0 To menuCount - 1
			y = y0 + i * lineH
			If my > y - lineH / 2 And my < y + lineH / 2 Then menuSel = i
		Next
	EndIf
	If KeyHit(200) Or JoyHit(5) Then menuSel = (menuSel + menuCount - 1) Mod menuCount
	If KeyHit(208) Or JoyHit(6) Then menuSel = (menuSel + 1) Mod menuCount
	If KeyHit(203) Then menuStep = -1
	If KeyHit(205) Then menuStep = 1
	If JoyType()
		If JoyYDir() <> joyLastY
			joyLastY = JoyYDir()
			If joyLastY < 0 Then menuSel = (menuSel + menuCount - 1) Mod menuCount
			If joyLastY > 0 Then menuSel = (menuSel + 1) Mod menuCount
		EndIf
		If JoyXDir() <> joyLastX
			joyLastX = JoyXDir()
			If joyLastX <> 0 Then menuStep = joyLastX
		EndIf
	EndIf

	SetFont fontMid
	For i = 0 To menuCount - 1
		y = y0 + i * lineH
		If i = menuSel
			Color 255, 200, 70
			Text GraphicsWidth() / 2, y, "> " + menuText(i) + " <", True, True
		Else
			Color 220, 225, 235
			Text GraphicsWidth() / 2, y, menuText(i), True, True
		EndIf
	Next

	activated = 0
	If KeyHit(28) Or KeyHit(156) Or JoyHit(1) Then activated = menuId(menuSel)
	If MouseHit(1)
		If my > y0 - lineH / 2 And my < y0 + (menuCount - 0.5) * lineH Then activated = menuId(menuSel)
	EndIf
	If menuStep <> 0 Then activated = menuId(menuSel)
	If activated Then Sfx(sndPaddle)
	Return activated
End Function

Function Title_Draw()
	SetFont fontBig
	Color 0, 0, 0 : Text GraphicsWidth() / 2 + U(4), U(110) + U(4), "BLOX'N'BALLS", True, True
	Color 240, 190, 60 : Text GraphicsWidth() / 2, U(110), "BLOX'N'BALLS", True, True
End Function

; ---- main menu ---------------------------------------------------------------

Function Menu_Update(dt#)
	Table_Orbit(dt)
	Effects_Update(dt)
	Blocks_Update(dt)
	If state = ST_MENU And gameActive
		If KeyHit(1) Then Menu_Resume()
	EndIf
End Function

Function Menu_Draw()
	Title_Draw()
	Menu_BuildMain()
	id = Menu_Run(U(260))
	Select id
	Case M_RESUME : Menu_Resume()
	Case M_CONTINUE
		Save_Load()
		If saveValid Then Game_New(saveSkill, saveLevel, saveScore, saveLives)
	Case M_NEW
		If menuStep = 0 Then Game_New(menuSkill, 1, 0, Skill_Lives(menuSkill))
	Case M_SKILL
		If menuStep < 0 Then menuSkill = (menuSkill + 2) Mod 3 Else menuSkill = (menuSkill + 1) Mod 3
	Case M_SCORES
		If menuStep = 0 Then scoreView = menuSkill : SetState(ST_SCORES)
	Case M_OPTIONS
		If menuStep = 0 Then menuSel = 0 : SetState(ST_OPTIONS)
	Case M_CREDITS
		If menuStep = 0 Then SetState(ST_CREDITS)
	Case M_QUIT
		If menuStep = 0 Then running = False
	End Select
End Function

Function Menu_Resume()
	Music_Play(lvMusic)
	SetState(ST_PLAY)
End Function

; ---- options -----------------------------------------------------------------

Function Options_Draw()
	Title_Draw()
	SetFont fontSmall : Color 160, 165, 175
	Text GraphicsWidth() / 2, U(190), "Left / Right or click to change", True, True
	Menu_BuildOptions()
	id = Menu_Run(U(260))
	s = menuStep
	If s = 0 Then s = 1
	Select id
	Case M_MUSIC : optMusic = Option_Step(optMusic, s * 0.1, 0, 1)
	Case M_SOUND : optSound = Option_Step(optSound, s * 0.1, 0, 1) : Sfx(sndBlock)
	Case M_MOUSE : optMouse = Option_Step(optMouse, s * 0.25, 0.25, 3)
	Case M_MIRROR : Table_SetMirror(Not optMirror)
	Case M_SIZE
		Select optWidth
		Case 1280 : If s > 0 Then optWidth = 1600 : optHeight = 900 Else optWidth = 1920 : optHeight = 1080
		Case 1600 : If s > 0 Then optWidth = 1920 : optHeight = 1080 Else optWidth = 1280 : optHeight = 720
		Default : If s > 0 Then optWidth = 1280 : optHeight = 720 Else optWidth = 1600 : optHeight = 900
		End Select
		Message("Window size changes after a restart", 3)
	Case M_FULL
		optFullscreen = Not optFullscreen
		Message("Fullscreen changes after a restart", 3)
	Case M_BACK
		If menuStep = 0 Then Options_Leave()
	End Select
	If KeyHit(1) Then Options_Leave()
	Message_Draw()
End Function

Function Options_Leave()
	Settings_Save()
	menuSel = 0
	SetState(ST_MENU)
End Function

; steps a value and wraps around at the ends
Function Option_Step#(v#, d#, lo#, hi#)
	v = v + d
	If v > hi + 0.001 Then v = lo
	If v < lo - 0.001 Then v = hi
	Return Clamp(v, lo, hi)
End Function

Function Message_Draw()
	If msgTime <= 0 Then Return
	SetFont fontSmall : Color 255, 220, 120
	Text GraphicsWidth() / 2, GraphicsHeight() - U(40), msgText, True, True
End Function

; ---- high scores ---------------------------------------------------------------

Function Scores_Draw()
	Title_Draw()
	If KeyHit(203) Then scoreView = (scoreView + 2) Mod 3
	If KeyHit(205) Or MouseHit(2) Then scoreView = (scoreView + 1) Mod 3
	SetFont fontMid : Color 255, 200, 70
	Text GraphicsWidth() / 2, U(200), "<  " + skillName(scoreView) + "  >", True, True
	SetFont fontSmall
	cx = GraphicsWidth() / 2
	For i = 0 To SCORE_MAX - 1
		y = U(260) + i * U(36)
		If i = nameRank And scoreView = gameSkill Then Color 255, 220, 120 Else Color 220, 225, 235
		Text cx - U(300), y, (i + 1) + ".", False, True
		Text cx - U(240), y, scoreName(scoreView, i), False, True
		Text cx + U(150), y, "Level " + scoreLevel(scoreView, i), True, True
		s$ = scoreValue(scoreView, i)
		Text cx + U(300) - StringWidth(s), y, s, False, True
	Next
	Color 160, 165, 175
	Text cx, GraphicsHeight() - U(40), "Left / Right: skill     Click / Esc: back", True, True
	If MouseHit(1) Or KeyHit(1) Or KeyHit(28) Or JoyHit(1) Or (autoPlay And stateTime > 3)
		nameRank = -1
		If autoPlay Then running = False
		SetState(ST_MENU)
	EndIf
End Function

; ---- name entry for a new high score ---------------------------------------------

Function Name_Begin()
	nameText = optLastName
	FlushKeys
	SetState(ST_NAME)
End Function

Function Name_Draw()
	Title_Draw()
	SetFont fontMid : Color 255, 200, 70
	Text GraphicsWidth() / 2, U(240), "New highscore: " + gameScore, True, True
	Color 220, 225, 235
	Text GraphicsWidth() / 2, U(320), "Enter your name", True, True
	k = GetKey()
	If k = 8 And Len(nameText) > 0 Then nameText = Left(nameText, Len(nameText) - 1)
	If k >= 32 And k < 127 And k <> 59 And Len(nameText) < 12 Then nameText = nameText + Chr(k)
	cursor$ = ""
	If (MilliSecs() / 400) Mod 2 Then cursor = "_"
	SetFont fontBig : Color 255, 255, 255
	Text GraphicsWidth() / 2, U(400), nameText + cursor, True, True
	If (k = 13 Or JoyHit(1)) And Trim(nameText) <> ""
		optLastName = Trim(nameText)
		Settings_Save()
		nameRank = Scores_Insert(gameSkill, optLastName, gameScore, gameLevel)
		scoreView = gameSkill
		SetState(ST_SCORES)
	EndIf
End Function

; ---- credits -------------------------------------------------------------------

Function Credits_Draw()
	Title_Draw()
	SetFont fontSmall : Color 220, 225, 235
	cx = GraphicsWidth() / 2 : y = U(220)
	Text cx, y, "Remaster of blox-n-balls, a Blitz3D game from 2003", True, True : y = y + U(40)
	Text cx, y, "Rewritten from scratch for BLTZNXT", True, True : y = y + U(40)
	Text cx, y, "github.com/Farbfinsternis/bltznxt", True, True : y = y + U(70)
	Color 160, 165, 175
	Text cx, y, "Click or Esc to go back", True, True
	If MouseHit(1) Or KeyHit(1) Or KeyHit(28) Or JoyHit(1) Then SetState(ST_MENU)
End Function

; ---- between levels and at the end -------------------------------------------------

Function LevelDone_Draw()
	cx = GraphicsWidth() / 2 : y = U(200)
	SetFont fontBig : Color 255, 210, 80
	Text cx, y, "Level " + gameLevel + " cleared!", True, True : y = y + U(100)
	SetFont fontMid : Color 220, 225, 235
	Text cx, y, "Level bonus   " + bonusLevel, True, True : y = y + U(50)
	Text cx, y, "Time bonus   " + bonusTime, True, True : y = y + U(50)
	Text cx, y, "Skill   x " + bonusFactor, True, True : y = y + U(70)
	Color 255, 210, 80
	Text cx, y, "+" + bonusTotal + "     Score " + gameScore, True, True
End Function

Function GameOver_Draw()
	cx = GraphicsWidth() / 2
	SetFont fontBig
	If gameWon
		Color 255, 210, 80 : Text cx, U(260), "All levels cleared!", True, True
	Else
		Color 230, 70, 60 : Text cx, U(260), "Game Over", True, True
	EndIf
	SetFont fontMid : Color 220, 225, 235
	Text cx, U(360), "Score " + gameScore + "     Level " + gameLevel, True, True
End Function
