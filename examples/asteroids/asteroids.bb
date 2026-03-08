; =============================================================
;  asteroids.bb - Asteroids Clone for BLTZNXT
;
;  Pure vector graphics, no external assets required.
;  Runs at 400x300 logical resolution, upscaled 2x to 800x600
;  via BLTZNXT mode 5 — demonstrating integer-scaled windowed
;  rendering without any changes to game coordinates.
;
;  Controls:
;    LEFT / RIGHT - rotate ship
;    UP            - thrust
;    SPACE         - fire / confirm menu
;    ESC           - menu / quit
; =============================================================

Global SCREEN_W% = 400
Global SCREEN_H% = 300

Const STATE_MENU     = 0
Const STATE_GAME     = 1
Const STATE_GAMEOVER = 2

Const KEY_ESC   = 1
Const KEY_SPACE = 57
Const KEY_LEFT  = 203
Const KEY_RIGHT = 205
Const KEY_UP    = 200

; ---- Types ----------------------------------------

Type TAsteroid
	Field ax#, ay#
	Field avx#, avy#
	Field aangle#
	Field arotspd#
	Field aradius#
	Field atier%
	Field adead%
End Type

Type TBullet
	Field bx#, by#
	Field bvx#, bvy#
	Field blife%
	Field bdead%
End Type

; ---- Player globals -----------------------------------

Global ship_x#, ship_y#, ship_vx#, ship_vy#, ship_angle#
Global ship_alive%

; ---- Game-state globals --------------------------------

Global score%, lives%, level%, hiscore%
Global gstate%
Global running%

; =============================================================
;  Helper: wrap a value into [lo, hi)
; =============================================================
Function fWrap#(v#, lo#, hi#)
	Local rng# = hi# - lo#
	While v# < lo#
		v# = v# + rng#
	Wend
	While v# >= hi#
		v# = v# - rng#
	Wend
	Return v#
End Function

; =============================================================
;  Helper: distance between two points
; =============================================================
Function fDist#(x1#, y1#, x2#, y2#)
	Local ddx# = x1 - x2
	Local ddy# = y1 - y2
	Return Sqr(ddx * ddx + ddy * ddy)
End Function

; =============================================================
;  Draw the player's ship (triangle + optional flame)
; =============================================================
Function DrawShip(cx#, cy#, ang#, thrust%)
	Color 255, 255, 255
	Local nx# = cx + Cos(ang) * 14
	Local ny# = cy - Sin(ang) * 14
	Local lx# = cx + Cos(ang + 135) * 10
	Local ly# = cy - Sin(ang + 135) * 10
	Local rx# = cx + Cos(ang - 135) * 10
	Local ry# = cy - Sin(ang - 135) * 10
	Line nx, ny, lx, ly
	Line nx, ny, rx, ry
	Line lx, ly, rx, ry
	If thrust Then
		Color 255, 100, 0
		Local flen% = Rand(8, 20)
		Local fx# = cx + Cos(ang + 180) * flen
		Local fy# = cy - Sin(ang + 180) * flen
		Line lx, ly, fx, fy
		Line rx, ry, fx, fy
	End If
End Function

; =============================================================
;  Draw one asteroid (irregular polygon)
; =============================================================
Function DrawAsteroid(dax#, day#, daangle#, daradius#, datier%)
	Color 200, 200, 200
	Local sides% = 9
	Local i%
	For i = 0 To sides - 1
		Local da1# = daangle + (i * 360.0 / sides)
		Local da2# = daangle + ((i + 1) * 360.0 / sides)
		Local dr1# = daradius * (0.78 + 0.22 * Sin(da1 * 2 + datier * 47))
		Local dr2# = daradius * (0.78 + 0.22 * Sin(da2 * 2 + datier * 47))
		Local dx1# = dax + Cos(da1) * dr1
		Local dy1# = day - Sin(da1) * dr1
		Local dx2# = dax + Cos(da2) * dr2
		Local dy2# = day - Sin(da2) * dr2
		Line dx1, dy1, dx2, dy2
	Next
End Function

; =============================================================
;  Spawn one asteroid.  fromx/fromy used when useSrc = 1 (split)
; =============================================================
Function SpawnAsteroid(tier%, fromx#, fromy#, useSrc%)
	Local na.TAsteroid = New TAsteroid
	If useSrc Then
		na\ax = fromx
		na\ay = fromy
	Else
		Local edge% = Rand(1, 4)
		If edge = 1 Then
			na\ax = Rnd(SCREEN_W)
			na\ay = 0
		End If
		If edge = 2 Then
			na\ax = Rnd(SCREEN_W)
			na\ay = SCREEN_H
		End If
		If edge = 3 Then
			na\ax = 0
			na\ay = Rnd(SCREEN_H)
		End If
		If edge = 4 Then
			na\ax = SCREEN_W
			na\ay = Rnd(SCREEN_H)
		End If
	End If
	na\atier    = tier
	Local spd# = 0.7 + level * 0.25 + (3 - tier) * 0.55
	Local dir# = Rnd(360)
	na\avx      = Cos(dir) * spd
	na\avy      = -Sin(dir) * spd
	na\aangle   = Rnd(360)
	na\arotspd  = (Rnd(4) - 2.0) * (4 - tier)
	If tier = 3 Then na\aradius = 38
	If tier = 2 Then na\aradius = 22
	If tier = 1 Then na\aradius = 11
	na\adead    = 0
End Function

; =============================================================
;  Fire a bullet from the ship nose
; =============================================================
Function FireBullet()
	Local nb.TBullet = New TBullet
	nb\bx    = ship_x + Cos(ship_angle) * 16
	nb\by    = ship_y - Sin(ship_angle) * 16
	Local spd# = 9.0
	nb\bvx   = ship_vx + Cos(ship_angle) * spd
	nb\bvy   = ship_vy - Sin(ship_angle) * spd
	nb\blife = 52
	nb\bdead = 0
End Function

; =============================================================
;  Delete all live game objects
; =============================================================
Function ClearAll()
	For Each ca.TAsteroid
		Delete ca
	Next
	For Each cb.TBullet
		Delete cb
	Next
End Function

; =============================================================
;  Count living asteroids
; =============================================================
Function CountAsteroids%()
	Local cnt% = 0
	For Each cc.TAsteroid
		If cc\adead = 0 Then cnt = cnt + 1
	Next
	Return cnt
End Function

; =============================================================
;  Populate the field with asteroids for the current level
; =============================================================
Function StartLevel()
	ClearAll()
	Local count% = 3 + level * 2
	Local i%
	For i = 1 To count
		SpawnAsteroid(3, 0, 0, 0)
	Next
End Function

; =============================================================
;  Respawn ship at screen centre
; =============================================================
Function ResetShip()
	ship_x     = SCREEN_W / 2
	ship_y     = SCREEN_H / 2
	ship_vx    = 0
	ship_vy    = 0
	ship_angle = 90
	ship_alive = 1
End Function

; =============================================================
;  MAIN
; =============================================================

AppTitle "Asteroids - BLTZNXT Edition"
Graphics SCREEN_W, SCREEN_H, 0, 5
SetBuffer BackBuffer()

gstate  = STATE_MENU
hiscore = 0
running = 1

While running

	Cls

	; ============================
	;  MAIN MENU
	; ============================
	If gstate = STATE_MENU Then

		Color 255, 255, 255
		Text SCREEN_W / 2, 110, "A S T E R O I D S", 1, 1

		Color 160, 200, 255
		Text SCREEN_W / 2, 168, "BLTZNXT Edition", 1, 1

		Color 255, 255, 80
		Text SCREEN_W / 2, 290, "SPACE  -  New Game", 1, 1

		Color 190, 190, 190
		Text SCREEN_W / 2, 345, "LEFT / RIGHT  -  Rotate", 1, 1
		Text SCREEN_W / 2, 375, "UP            -  Thrust", 1, 1
		Text SCREEN_W / 2, 405, "SPACE         -  Fire", 1, 1
		Text SCREEN_W / 2, 435, "ESC           -  Quit", 1, 1

		If hiscore > 0 Then
			Color 255, 200, 50
			Text SCREEN_W / 2, 500, "Hi-Score:  " + hiscore, 1, 1
		End If

		If KeyHit(KEY_SPACE) Then
			score  = 0
			lives  = 3
			level  = 1
			ResetShip()
			StartLevel()
			gstate = STATE_GAME
		End If

		If KeyDown(KEY_ESC) Then
			running = 0
		End If

	; ============================
	;  GAME
	; ============================
	ElseIf gstate = STATE_GAME Then

		; ---- Input ----------------------------------------
		Local thrusting% = 0
		If ship_alive Then
			If KeyDown(KEY_LEFT)  Then ship_angle = ship_angle + 4
			If KeyDown(KEY_RIGHT) Then ship_angle = ship_angle - 4
			If KeyDown(KEY_UP) Then
				ship_vx   = ship_vx + Cos(ship_angle) * 0.28
				ship_vy   = ship_vy - Sin(ship_angle) * 0.28
				thrusting = 1
			End If
			If KeyHit(KEY_SPACE) Then FireBullet()
		End If

		; ---- Physics: ship --------------------------------
		ship_vx = ship_vx * 0.988
		ship_vy = ship_vy * 0.988
		If ship_alive Then
			ship_x = fWrap(ship_x + ship_vx, 0, SCREEN_W)
			ship_y = fWrap(ship_y + ship_vy, 0, SCREEN_H)
		End If

		; ---- Physics: bullets -----------------------------
		For Each ub.TBullet
			ub\bx    = fWrap(ub\bx + ub\bvx, 0, SCREEN_W)
			ub\by    = fWrap(ub\by + ub\bvy, 0, SCREEN_H)
			ub\blife = ub\blife - 1
			If ub\blife <= 0 Then ub\bdead = 1
		Next

		; ---- Physics: asteroids ---------------------------
		For Each ua.TAsteroid
			ua\ax     = fWrap(ua\ax + ua\avx, 0, SCREEN_W)
			ua\ay     = fWrap(ua\ay + ua\avy, 0, SCREEN_H)
			ua\aangle = ua\aangle + ua\arotspd
		Next

		; ---- Collision: bullet vs asteroid ----------------
		For Each cb.TBullet
			If cb\bdead = 0 Then
				For Each ca.TAsteroid
					If ca\adead = 0 Then
						If fDist(cb\bx, cb\by, ca\ax, ca\ay) < ca\aradius Then
							cb\bdead = 1
							ca\adead = 1
							If ca\atier = 3 Then score = score + 20
							If ca\atier = 2 Then score = score + 50
							If ca\atier = 1 Then score = score + 100
						End If
					End If
				Next
			End If
		Next

		; ---- Collision: ship vs asteroid ------------------
		If ship_alive Then
			For Each sa.TAsteroid
				If sa\adead = 0 Then
					If fDist(ship_x, ship_y, sa\ax, sa\ay) < sa\aradius + 11 Then
						ship_alive = 0
						lives      = lives - 1
					End If
				End If
			Next
		End If

		; ---- Cleanup: dead asteroids (spawn children) -----
		For Each da.TAsteroid
			If da\adead Then
				If da\atier > 1 Then
					SpawnAsteroid(da\atier - 1, da\ax, da\ay, 1)
					SpawnAsteroid(da\atier - 1, da\ax, da\ay, 1)
				End If
				Delete da
			End If
		Next

		; ---- Cleanup: dead bullets ------------------------
		For Each db.TBullet
			If db\bdead Then Delete db
		Next

		; ---- Respawn ship ---------------------------------
		If ship_alive = 0 And lives > 0 Then
			ResetShip()
		End If

		; ---- Level complete? ------------------------------
		If CountAsteroids() = 0 Then
			level = level + 1
			StartLevel()
		End If

		; ---- Game over? -----------------------------------
		If lives <= 0 Then
			If score > hiscore Then hiscore = score
			gstate = STATE_GAMEOVER
		End If

		; ---- Draw: asteroids ------------------------------
		For Each rd.TAsteroid
			DrawAsteroid(rd\ax, rd\ay, rd\aangle, rd\aradius, rd\atier)
		Next

		; ---- Draw: bullets --------------------------------
		Color 255, 255, 80
		For Each rb.TBullet
			Plot rb\bx,     rb\by
			Plot rb\bx + 1, rb\by
			Plot rb\bx,     rb\by + 1
			Plot rb\bx + 1, rb\by + 1
		Next

		; ---- Draw: ship -----------------------------------
		If ship_alive Then
			DrawShip(ship_x, ship_y, ship_angle, thrusting)
		End If

		; ---- HUD ------------------------------------------
		Color 255, 255, 255
		Text 8,              8,  "SCORE  " + score
		Text 8,              28, "LEVEL  " + level
		Text SCREEN_W - 8,   8,  "LIVES  " + lives, 1, 0

		; ESC → menu
		If KeyHit(KEY_ESC) Then
			ClearAll()
			gstate = STATE_MENU
		End If

	; ============================
	;  GAME OVER
	; ============================
	ElseIf gstate = STATE_GAMEOVER Then

		Color 255, 60, 60
		Text SCREEN_W / 2, 190, "G A M E   O V E R", 1, 1

		Color 255, 255, 255
		Text SCREEN_W / 2, 270, "Final Score:  " + score, 1, 1

		If score >= hiscore And score > 0 Then
			Color 255, 215, 50
			Text SCREEN_W / 2, 315, "New Hi-Score!", 1, 1
		End If

		Color 200, 200, 200
		Text SCREEN_W / 2, 410, "SPACE  -  Play Again", 1, 1
		Text SCREEN_W / 2, 450, "ESC    -  Main Menu", 1, 1

		If KeyHit(KEY_SPACE) Then
			score  = 0
			lives  = 3
			level  = 1
			ResetShip()
			StartLevel()
			gstate = STATE_GAME
		End If

		If KeyHit(KEY_ESC) Then
			gstate = STATE_MENU
		End If

	End If

	Flip

Wend

EndGraphics
End
