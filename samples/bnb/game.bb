; game.bb - one game: levels, clock, lives and score.

Global gameActive = False   ; a game is running (the menu offers "Resume")
Global gameSkill = 1
Global gameLevel = 1
Global gameScore = 0
Global gameLives = 4
Global levelCount = 0
Global timeLeft#, timeFull#
Global levelElapsed#
Global nudgeCooldown#
Global launchDelay#         ; autopilot: wait before launching
Global msgText$ = "", msgTime# = 0

; level done: bonus tally
Global bonusLevel, bonusTime, bonusFactor#, bonusTotal

Dim skillName$(2)
skillName(0) = "Easy" : skillName(1) = "Normal" : skillName(2) = "Hard"

Function Skill_Lives(s)
	Return 5 - s                        ; 5 / 4 / 3
End Function

Function Skill_Speed#(s)
	Select s
	Case 0 : Return 0.85
	Case 2 : Return 1.2
	End Select
	Return 1.0
End Function

Function Skill_Time#(s)
	Select s
	Case 0 : Return 1.25
	Case 2 : Return 0.8
	End Select
	Return 1.0
End Function

Function Skill_Paddle#(s)
	Select s
	Case 0 : Return 1.45
	Case 2 : Return 1.0
	End Select
	Return 1.2
End Function

; fromSave = continue from the autosave (also brings back its inventory)
Function Game_New(skill, level, score, lives, coins = 0, fromSave = False)
	gameSkill = skill : gameScore = score : gameLives = lives : gameCoins = coins
	gameActive = True
	Extras_Reset(True)
	If fromSave Then Save_RestoreInventory()
	levelCount = Level_Count()
	Game_StartLevel(level)
End Function

Function Game_StartLevel(n)
	gameLevel = n
	If Not Level_Load(n)
		Game_End(True)
		Return
	EndIf
	Blocks_Build()
	Balls_Clear()
	Coins_Clear()
	Effects_Clear()
	paddleX = 0 : paddleVel = 0
	Extras_Reset(False)
	Paddle_SetWidth(Skill_Paddle(gameSkill))
	Ball_Serve()
	timeFull = lvTime * Skill_Time(gameSkill)
	timeLeft = timeFull
	levelElapsed = 0
	launchDelay = 0.8
	Music_Play(lvMusic)
	If Not autoPlay Then Save_Write(gameSkill, n, gameScore, gameLives, gameCoins)   ; test runs keep the real save
	Message(lvName, 2.5)
	SetState(ST_PLAY)
End Function

Function Message(t$, seconds#)
	msgText = t : msgTime = seconds
End Function

Function Play_Update(dt#)
	If KeyHit(1) Or JoyHit(8)
		SetState(ST_MENU)
		Return
	EndIf
	If KeyHit(15) Then ballcamOn = Not ballcamOn
	; inventory: wheel, arrow keys or gamepad shoulders select,
	; right button / Ctrl / button 2 use
	wheel = MouseZSpeed()
	If wheel > 0 Or KeyHit(200) Or JoyHit(5) Then Inv_Select(-1)
	If wheel < 0 Or KeyHit(208) Or JoyHit(6) Then Inv_Select(1)
	If MouseHit(2) Or KeyHit(29) Or KeyHit(157) Or JoyHit(2) Then Inv_Use()
	If nudgeCooldown > 0 Then nudgeCooldown = nudgeCooldown - dt
	If (KeyHit(57) Or JoyHit(4)) And nudgeCooldown <= 0
		Balls_Nudge()
		Table_Shake(0.25)
		Sfx(sndBumper)
		nudgeCooldown = 1.5
	EndIf
	If debugMode
		If KeyHit(59) Then Blocks_Clear()                    ; F1: clear the level
		If KeyHit(60) Then gameLives = gameLives + 1         ; F2: one more ball
	EndIf

	; paddle: mouse, gamepad or autopilot
	oldPaddleX# = paddleX
	If Not autoPlay
		mx = MouseXSpeed()
		MoveMouse GraphicsWidth() / 2, GraphicsHeight() / 2
		Paddle_Move(mx * 0.022 * optMouse)
	EndIf
	If JoyType() And Abs(JoyX()) > 0.15 Then Paddle_Move(JoyX() * 20 * dt)
	If autoPlay Then Autopilot(dt)
	If dt > 0 Then paddleVel = paddleVel * 0.5 + (paddleX - oldPaddleX) / dt * 0.5

	launch = MouseHit(1) Or JoyHit(1)
	If autoPlay
		launchDelay = launchDelay - dt
		If launchDelay <= 0 Then launch = True
	EndIf
	If launch Then Balls_Launch() : launchDelay = 0.8

	; the clock runs while a ball is moving
	moving = False
	For b.tBall = Each tBall
		If Not b\stuck Then moving = True
	Next
	If moving
		timeLeft = timeLeft - dt
		levelElapsed = levelElapsed + dt
	EndIf
	ballSpeed = lvSpeed * Skill_Speed(gameSkill) * (1 + Clamp(levelElapsed / 150, 0, 0.4)) * Extras_SpeedFactor()

	gameScore = gameScore + Balls_Update(dt)
	Extras_Update(dt)
	Coins_Update(dt)
	Blocks_Update(dt)
	Effects_Update(dt)
	If msgTime > 0 Then msgTime = msgTime - dt

	If blocksLeft <= 0
		Level_Done()
		Return
	EndIf

	If timeLeft <= 0
		; time out costs a ball, the clock starts again
		Balls_Clear()
		timeLeft = timeFull
		Message("Time out", 2)
		Ball_Lost()
	ElseIf Balls_Count() = 0
		Ball_Lost()
	EndIf

	leader.tBall = Ball_Leader()
	If leader <> Null
		Table_Update(dt, leader\x, leader\z, leader\vx, leader\vz)
	Else
		Table_Update(dt, paddleX, PADDLE_Z, 0, 1)
	EndIf
End Function

Function Ball_Lost()
	If debugMode Then DebugLog Int(levelElapsed) + "s ball lost, lives " + (gameLives - 1) + " speed " + Int(ballSpeed) + " tSpeed " + Int(tSpeed)
	gameLives = gameLives - 1
	Sfx(sndLoseBall)
	Table_Shake(0.3)
	If gameLives <= 0
		Game_End(False)
	Else
		Ball_Serve()
		launchDelay = 0.8
	EndIf
End Function

Function Level_Done()
	Sfx(sndWin)
	Balls_Clear()
	Coins_Clear()               ; coins the ball did not pick up are lost
	Table_BallcamOff()
	bonusLevel = 1000
	bonusTime = Int(timeLeft) * 20
	bonusFactor = 1 + gameSkill * 0.5
	bonusTotal = Int((bonusLevel + bonusTime) * bonusFactor)
	gameScore = gameScore + bonusTotal
	SetState(ST_LEVELDONE)
End Function

Function LevelDone_Update(dt#)
	Effects_Update(dt)
	Table_Update(dt, 0, 10, 0, 1)
	If stateTime > 1 And (MouseHit(1) Or KeyHit(28) Or KeyHit(57) Or JoyHit(1)) Then LevelDone_Next()
	If stateTime > 6 Or (autoPlay And stateTime > 2) Then LevelDone_Next()
End Function

Function LevelDone_Next()
	If gameLevel >= levelCount
		Game_End(True)
	Else
		Shop_Open()
	EndIf
End Function

; won = all levels cleared
Function Game_End(won)
	gameActive = False
	If Not autoPlay Then Save_Clear()
	Balls_Clear()
	Coins_Clear()
	Table_BallcamOff()
	If won
		Sfx(sndWin)
	Else
		Sfx(sndLose)
	EndIf
	gameWon = won
	SetState(ST_GAMEOVER)
End Function

Global gameWon = False

Function GameOver_Update(dt#)
	Effects_Update(dt)
	Blocks_Update(dt)
	Table_Orbit(dt)
	pressed = MouseHit(1) Or KeyHit(28) Or KeyHit(1) Or JoyHit(1)
	If (stateTime > 1.5 And pressed) Or (autoPlay And stateTime > 3)
		Blocks_Clear()
		Effects_Clear()
		If Scores_Rank(gameSkill, gameScore) >= 0 And (Not autoPlay)
			Name_Begin()
		Else
			scoreView = gameSkill
			SetState(ST_SCORES)
		EndIf
	EndIf
End Function

; autopilot for tests (-autoplay): follow the ball, hit it a bit off centre
Global autoAim# = 0
Function Autopilot(dt#)
	target.tBall = Null
	For b.tBall = Each tBall
		If b\vz < 0 And (target = Null)
			target = b
		ElseIf b\vz < 0
			If b\z < target\z Then target = b
		EndIf
	Next
	If target = Null Then target = Ball_Leader()
	If target = Null Then Return
	If Rand(60) = 1 Then autoAim = Rnd(-0.7, 0.7) * paddleHW
	If invCount > 0 And Rand(180) = 1 Then invSel = 0 : Inv_Use()
	dx# = target\x - autoAim - paddleX
	Paddle_Move(Clamp(dx, -25 * dt, 25 * dt))
End Function
