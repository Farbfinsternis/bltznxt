; Blox'n'Balls - remaster of the Blitz3D game blox-n-balls (2003),
; rewritten from scratch as a BLTZNXT sample.
;
; Version 0.1 (in progress): table, paddle, ball, blocks, HUD, winning and
; losing, the five original levels, the extras of the special blocks and
; the inventory, coins and the shop between the levels. The level editor
; follows. Requirements: FRAGENKATALOG.md, engine findings: PROTOKOLL.md.
;
; Controls:  mouse moves the paddle, left button launches the ball
;            Space nudges the table, Tab toggles the ball camera
;            mouse wheel selects an item, right button (or Ctrl) uses it
;            Esc opens the menu (the game waits), F12 saves a screenshot
;            Gamepad: stick moves, button 1 launches, button 4 nudges,
;            buttons 5 / 6 select an item, button 2 uses it
;
; Command line (for tests and debugging):
;   -debug        F1 clears the level, F2 adds a ball, frame time on screen
;   -autoplay     starts a game at once, the paddle plays by itself
;   -level <n>    first level for -autoplay (default 1)
;   -skill <n>    0 easy, 1 normal, 2 hard (default 1)
;   -quit <s>     quits after s seconds and saves test_quit.bmp
;   -ballcam      starts with the ball camera on
;   -shots <s>    saves test_001.bmp, test_002.bmp ... every s seconds
;   -shop         starts in the shop with 20 coins, then plays by itself

Const ST_MENU = 1
Const ST_PLAY = 2
Const ST_LEVELDONE = 3
Const ST_GAMEOVER = 4
Const ST_SCORES = 5
Const ST_OPTIONS = 6
Const ST_CREDITS = 7
Const ST_NAME = 8
Const ST_SHOP = 9

Global state = 0
Global stateTime# = 0
Global running = True
Global debugMode = False
Global autoPlay = False

Include "settings.bb"
Include "level.bb"
Include "table.bb"
Include "sound.bb"
Include "effects.bb"
Include "blocks.bb"
Include "ball.bb"
Include "extras.bb"
Include "coins.bb"
Include "hud.bb"
Include "game.bb"
Include "menu.bb"
Include "shop.bb"

; ---- command line ----------------------------------------------------------------

startLevel = 1 : startSkill = 1 : quitAfter# = 0 : shotEvery# = 0 : shotNext# = 0 : shotCount = 0 : startShop = False
args$ = Trim(CommandLine())
i = 1
Repeat
	arg$ = Part$(Replace(args, " ", ";"), i)
	If arg = "" Then Exit
	Select Lower(arg)
	Case "-debug"    : debugMode = True
	Case "-autoplay" : autoPlay = True
	Case "-ballcam"  : ballcamOn = True
	Case "-shots"    : i = i + 1 : shotEvery = Float(Part$(Replace(args, " ", ";"), i))
	Case "-shop"     : startShop = True : autoPlay = True
	Case "-level"    : i = i + 1 : startLevel = Int(Part$(Replace(args, " ", ";"), i))
	Case "-skill"    : i = i + 1 : startSkill = Int(Part$(Replace(args, " ", ";"), i))
	Case "-quit"     : i = i + 1 : quitAfter = Float(Part$(Replace(args, " ", ";"), i))
	End Select
	i = i + 1
Forever
If startLevel < 1 Then startLevel = 1
startSkill = Int(Clamp(startSkill, 0, 2))

; ---- start -------------------------------------------------------------------------

Settings_Load()
Scores_Load()
Save_Load()

AppTitle "Blox'n'Balls"
If optFullscreen
	Graphics3D optWidth, optHeight, 0, 1
Else
	Graphics3D optWidth, optHeight, 0, 2
EndIf
SetBuffer BackBuffer()
SeedRnd MilliSecs()

; loading screen while the rest loads
MainImage_Load()
Cls : MainImage_Draw() : Flip

Fonts_Load()
Sound_Load()
Table_Create()
Effects_Create()
Ball_Create()
Blocks_Init()
Coins_Create()
Shop_Init()
HUD_Create()
menuSkill = startSkill

If autoPlay
	Game_New(startSkill, startLevel, 0, Skill_Lives(startSkill))
	If startShop Then gameCoins = 20 : Blocks_Clear() : Balls_Clear() : Shop_Open()
Else
	SetState(ST_MENU)
EndIf

; ---- main loop -----------------------------------------------------------------------

totalTime# = 0
lastMs = MilliSecs()
While running
	now = MilliSecs()
	dt# = (now - lastMs) / 1000.0
	lastMs = now
	If dt > 0.05 Then dt = 0.05
	stateTime = stateTime + dt
	totalTime = totalTime + dt
	shot = KeyHit(88)
	Music_Update()

	Select state
	Case ST_PLAY      : Play_Update(dt)
	Case ST_LEVELDONE : LevelDone_Update(dt)
	Case ST_GAMEOVER  : GameOver_Update(dt)
	Case ST_SHOP      : Shop_Update(dt)
	Default           : Menu_Update(dt)
	End Select

	RenderWorld

	Select state
	Case ST_PLAY      : Effects_Draw() : HUD_Draw()
	Case ST_LEVELDONE : Effects_Draw() : LevelDone_Draw()
	Case ST_GAMEOVER  : GameOver_Draw()
	Case ST_MENU      : Menu_Draw()
	Case ST_OPTIONS   : Options_Draw()
	Case ST_SCORES    : Scores_Draw()
	Case ST_CREDITS   : Credits_Draw()
	Case ST_NAME      : Name_Draw()
	Case ST_SHOP      : Shop_Draw()
	End Select
	If msgTime > 0 And state <> ST_PLAY Then msgTime = msgTime - dt

	If debugMode
		SetFont fontSmall : Color 160, 255, 160
		Text U(8), GraphicsHeight() - U(24), Int(dt * 1000) + " ms  balls " + Balls_Count() + "  blocks " + blocksLeft + "  sparks " + sparkCount
	EndIf

	If shot Then Screenshot()
	If shotEvery > 0 And totalTime >= shotNext
		shotNext = totalTime + shotEvery : shotCount = shotCount + 1
		SaveBuffer BackBuffer(), AppPath("test_" + Right("00" + shotCount, 3) + ".bmp")
	EndIf
	If quitAfter > 0 And totalTime >= quitAfter
		SaveBuffer BackBuffer(), AppPath("test_quit.bmp")
		DebugLog "quit: state " + state + " level " + gameLevel + " score " + gameScore + " lives " + gameLives + " blocks " + blocksLeft + " max z " + ballMaxZ
		running = False
	EndIf
	Flip
Wend

Settings_Save()
End

; ---- states --------------------------------------------------------------------------

Function SetState(s)
	previous = state
	state = s : stateTime = 0
	FlushMouse
	HUD_Show(s = ST_PLAY)
	If s = ST_PLAY
		If Not autoPlay
			HidePointer
			MoveMouse GraphicsWidth() / 2, GraphicsHeight() / 2
		EndIf
		MouseXSpeed() : MouseYSpeed()
		leader.tBall = Ball_Leader()
		If leader <> Null Then Table_SnapView(leader\x, leader\z) Else Table_SnapView(0, 0)
		Return
	EndIf
	ShowPointer
	If s <> ST_LEVELDONE Then Music_Play("mainmusic.mp3")
	If previous = ST_PLAY Then Table_BallcamOff()
	; an empty table looks dull behind the menu: show the first level
	If s = ST_MENU And (Not gameActive) And blocksLeft = 0
		If Level_Load(1) Then Blocks_Build()
	EndIf
End Function

Function Screenshot()
	n = 0
	While FileType(AppPath("shot" + Right("00" + n, 3) + ".bmp")) = 1
		n = n + 1
	Wend
	SaveBuffer BackBuffer(), AppPath("shot" + Right("00" + n, 3) + ".bmp")
End Function
