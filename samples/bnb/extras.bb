; extras.bb - what the special blocks do, timed effects and the inventory.
;
; Most extras act at once (multiball, extra ball, time, new blocks) or for
; EXTRA_TIME seconds (paddle and ball size, speed, score x2). The atomic
; ball goes into the inventory: the mouse wheel selects, the right button
; uses an item. The shop will fill the inventory as well.

Const EXTRA_TIME# = 10
Const DOUBLE_TIME# = 20
Const INV_MAX = 20

Global tSmallPaddle#, tWidePaddle#, wideCount
Global tTinyBall#, tBigBall#, tSlow#, tSpeed#, tDouble#
Global atomicArmed = False
Global paddleWanted# = 1.2          ; half width the extras ask for

Dim invItem$(INV_MAX - 1)
Global invCount = 0, invSel = 0

; full = new game (also empties the inventory), otherwise new level
Function Extras_Reset(full)
	tSmallPaddle = 0 : tWidePaddle = 0 : wideCount = 0
	tTinyBall = 0 : tBigBall = 0 : tSlow = 0 : tSpeed = 0 : tDouble = 0
	atomicArmed = False
	If full Then invCount = 0 : invSel = 0
	Ball_SetRadius(BALL_R_NORMAL)
End Function

; the ball destroyed special block <code> at x, z; returns the text that
; rises from the block
Function Extra_Trigger$(code$, x#, z#)
	Select code
	Case "M", "N", "O"
		n = Asc(code) - Asc("M") + 1
		For i = 1 To n
			a# = Rnd(-140, 140)
			Ball_Add(x, z, Sin(a) * ballSpeed, Cos(a) * ballSpeed, False)
		Next
		Sparks(20, x, 0.5, z, 120, 200, 255, 5)
		Return "Multiball +" + n
	Case "E"
		gameLives = gameLives + 1
		Return "Extra ball"
	Case "A"
		Inv_Add("Atomic ball")
		Return "Atomic ball"
	Case "S"
		tSpeed = EXTRA_TIME : tSlow = 0
		Return "Speed ball"
	Case "B"
		Return Blocks_Respawn(x, z) + " new blox"
	Case "s"
		tSmallPaddle = EXTRA_TIME : tWidePaddle = 0 : wideCount = 0
		Return "Small paddle"
	Case "W"
		If wideCount < 2 Then wideCount = wideCount + 1
		tWidePaddle = EXTRA_TIME : tSmallPaddle = 0
		Return "Wide paddle"
	Case "t"
		tTinyBall = EXTRA_TIME : tBigBall = 0
		Return "Tiny ball"
	Case "G"
		tBigBall = EXTRA_TIME : tTinyBall = 0
		Return "Bowling!"
	Case "L"
		tSlow = EXTRA_TIME : tSpeed = 0
		Return "Slow motion"
	Case "X"
		tDouble = DOUBLE_TIME
		Return "Score x2"
	Case "T"
		timeLeft = timeLeft + 15
		Return "Time +15"
	Case "U"
		timeLeft = timeLeft - 15
		If timeLeft < 1 Then timeLeft = 1
		Return "Time -15"
	End Select
	Return ""
End Function

Function Extras_Update(dt#)
	tSmallPaddle = Extras_Tick(tSmallPaddle, dt)
	tWidePaddle = Extras_Tick(tWidePaddle, dt)
	If tWidePaddle <= 0 Then wideCount = 0
	tTinyBall = Extras_Tick(tTinyBall, dt)
	tBigBall = Extras_Tick(tBigBall, dt)
	tSlow = Extras_Tick(tSlow, dt)
	tSpeed = Extras_Tick(tSpeed, dt)
	tDouble = Extras_Tick(tDouble, dt)

	; paddle width and ball size follow the running effects
	w# = Skill_Paddle(gameSkill)
	If tSmallPaddle > 0 Then w = w * 0.55
	For i = 1 To wideCount : w = w * 1.5 : Next
	If Abs(w - paddleHW) > 0.001
		; grow and shrink smoothly
		Paddle_SetWidth(paddleHW + Clamp(w - paddleHW, -4 * dt, 4 * dt))
	EndIf
	r# = BALL_R_NORMAL
	If tTinyBall > 0 Then r = r * 0.55
	If tBigBall > 0 Then r = r * 1.8
	If Abs(r - ballR) > 0.001 Then Ball_SetRadius(r)
End Function

Function Extras_Tick#(t#, dt#)
	If t <= 0 Then Return 0
	t = t - dt
	If t < 0 Then t = 0
	Return t
End Function

Function Extras_SpeedFactor#()
	f# = 1
	If tSlow > 0 Then f = f * 0.6
	If tSpeed > 0 Then f = f * 1.5
	Return f
End Function

Function Extras_ScoreFactor()
	If tDouble > 0 Then Return 2
	Return 1
End Function

; ---- inventory -------------------------------------------------------------------

Function Inv_Add(name$)
	If invCount >= INV_MAX Then Return
	invItem(invCount) = name
	invSel = invCount
	invCount = invCount + 1
End Function

Function Inv_Select(delta)
	If invCount = 0 Then Return
	invSel = (invSel + delta + invCount) Mod invCount
End Function

Function Inv_Current$()
	If invCount = 0 Then Return ""
	Return invItem(invSel)
End Function

Function Inv_Use()
	If invCount = 0 Then Return
	name$ = invItem(invSel)
	Select name
	Case "Atomic ball"
		If atomicArmed Then Return              ; one at a time
		atomicArmed = True
	Case "Extra ball"
		gameLives = gameLives + 1
	Case "Time +15"
		timeLeft = timeLeft + 15
	Case "Wide paddle"
		If wideCount < 2 Then wideCount = wideCount + 1
		tWidePaddle = EXTRA_TIME : tSmallPaddle = 0
	Case "Slow motion"
		tSlow = EXTRA_TIME : tSpeed = 0
	End Select
	Sfx(sndBonus)
	For i = invSel To invCount - 2
		invItem(i) = invItem(i + 1)
	Next
	invCount = invCount - 1
	If invSel >= invCount Then invSel = 0
	FloatText(paddleX, PADDLE_Z + 0.8, name, 140, 220, 255)
End Function
