; Leuchtturm - der Spieler: Bewegung und Kollision (Schritt 3).
;
; Das Ziel ist das Gefuehl von Quake III: schnelles Laufen, das sofort
; anspricht, ein Sprung von fast anderthalb Metern, und in der Luft genug Kontrolle,
; um mit Strafe und Maus die Richtung zu biegen. Dafuer fuehrt das Programm
; die Geschwindigkeit selbst:
;
;   - am Boden bremst Reibung, unter SP_STOPP so stark wie bei SP_STOPP -
;     man bleibt also rasch stehen, statt auszurollen
;   - Beschleunigung zur Wunschrichtung, am Boden stark, in der Luft
;     schwach; sie fuellt nur bis zur Laufgeschwindigkeit *in dieser
;     Richtung* auf - quer dazu bleibt vorhandener Schwung erhalten, daraus
;     entsteht das Strafe-Springen
;   - Sprung nur vom Boden, Schwerkraft immer
;
; Die Kollision loest UpdateWorld: der Koerper ist ein Ellipsoid (0.4 breit,
; 0.9 halbe Hoehe) gegen die Dreiecke der Karte, mit Antwort 3 - Gleiten,
; aber nur so weit, wie man sich waagrecht bewegt hat: wer steht, rutscht
; keine Rampe hinab. Danach wird die Geschwindigkeit an jeder beruehrten
; Flaeche abgeschnitten (sonst drueckt sie weiter gegen die Wand), und eine
; Flaeche, die flacher als 45 Grad ist, gilt als Boden.
;
; Boden: Die Kollision allein reicht nicht - wer gegen eine Wand laeuft,
; bekommt in diesem Takt keinen Bodenkontakt gemeldet (das Gleiten an der
; Wand verbraucht die Bewegung). Deshalb prueft nach jedem Takt eine Kugel
; knapp unter den Fuessen (LinePick gegen die "-col"-Geometrie), ob Boden
; da ist, und zieht die Fuesse bis zu SP_SCHNAPP hinunter - so hebt man
; beim Abwaertslaufen ueber Rampen und Stufen nicht ab. Am Boden wirkt keine
; Schwerkraft.
;
; Je Takt (60 je Sekunde):
;
;   Spieler_Takt p, vor, seit, springen     ; -1..1, -1..1, True/False
;   UpdateWorld
;   Spieler_Nach p
;
; Masse in Metern und Sekunden; die Fuesse stehen 0.9 unter der Mitte.

; Die Werte sind die von Quake III, umgerechnet ueber die Groesse des
; Spielers: 56 Quake-Einheiten sind 1.8 m, eine Einheit also 3.2 cm. Laufen
; 320 u/s, Absprung 270 u/s, Schwerkraft 800 u/s2, Anhalten 100 u/s;
; Reibung und Beschleunigungen haben keine Einheit und bleiben.
Const SP_RADIUS#   = 0.4    ; Ellipsoid: halbe Breite
Const SP_HALB#     = 0.9    ; halbe Hoehe
Const SP_AUGE#     = 0.7    ; Auge ueber der Mitte, 1.6 ueber den Fuessen
Const SP_LAUF#     = 10.3   ; Laufgeschwindigkeit
Const SP_BESCHL#   = 10.0   ; Beschleunigung am Boden (je Sekunde, mal Laufgeschwindigkeit)
Const SP_LUFT#     = 1.0    ; dasselbe in der Luft
Const SP_REIBUNG#  = 6.0
Const SP_STOPP#    = 3.2
Const SP_SPRUNG#   = 8.7    ; Absprung: v*v / (2*g) = 1.47 m
Const SP_SCHWERE#  = 25.7
Const SP_BODEN_NY# = 0.7    ; Normale mit mehr Y ist Boden (flacher als 45 Grad)
Const SP_FUSS_R#   = 0.3    ; Kugel der Bodenpruefung
Const SP_SCHNAPP#  = 0.1    ; so weit werden die Fuesse zum Boden gezogen
Const TAKT#        = 1.0 / 60.0

Type Spieler
	Field koerper, kamera
	Field vx#, vy#, vz#
	Field boden
	Field gier#, nick#
End Type

Function Spieler_Neu.Spieler(x#, y#, z#, gier#)
	p.Spieler = New Spieler
	p\koerper = CreatePivot()
	EntityRadius p\koerper, SP_RADIUS, SP_HALB
	EntityType p\koerper, TYP_SPIELER
	p\kamera = CreateCamera(p\koerper)
	PositionEntity p\kamera, 0, SP_AUGE, 0
	CameraRange p\kamera, 0.05, 1000
	Collisions TYP_SPIELER, TYP_WELT, 2, 3
	Spieler_Setzen(p, x, y, z, gier)
	Return p
End Function

; An einen Ort stellen, (x,y,z) sind die Fuesse. Ohne Schwung.
Function Spieler_Setzen(p.Spieler, x#, y#, z#, gier#)
	PositionEntity p\koerper, x, y + SP_HALB, z
	ResetEntity p\koerper
	p\vx = 0 : p\vy = 0 : p\vz = 0
	p\boden = False
	p\gier = gier : p\nick = 0
	RotateEntity p\koerper, 0, p\gier, 0
	RotateEntity p\kamera, 0, 0, 0
End Function

; Maus: Drehen um die Hochachse am Koerper, Nicken nur mit der Kamera.
Function Spieler_Schauen(p.Spieler, dgier#, dnick#)
	p\gier = p\gier + dgier
	p\nick = p\nick + dnick
	If p\nick > 89 Then p\nick = 89
	If p\nick < -89 Then p\nick = -89
	RotateEntity p\koerper, 0, p\gier, 0
	RotateEntity p\kamera, p\nick, 0, 0
End Function

Function Spieler_Takt(p.Spieler, vor#, seit#, springen)
	; Wunschrichtung waagrecht in der Welt; schraeg ist nicht schneller.
	TFormVector seit, 0, vor, p\koerper, 0
	wx# = TFormedX() : wz# = TFormedZ()
	l# = Sqr(wx * wx + wz * wz)
	wunsch# = 0
	If l > 0
		wx = wx / l : wz = wz / l
		If l > 1 Then l = 1
		wunsch = SP_LAUF * l
	EndIf

	If p\boden
		v# = Sqr(p\vx * p\vx + p\vz * p\vz)
		If v > 0
			bremse# = v : If bremse < SP_STOPP Then bremse = SP_STOPP
			neu# = v - bremse * SP_REIBUNG * TAKT
			If neu < 0 Then neu = 0
			p\vx = p\vx * neu / v : p\vz = p\vz * neu / v
		EndIf
	EndIf

	If wunsch > 0
		If p\boden Then a# = SP_BESCHL Else a# = SP_LUFT
		fehlt# = wunsch - (p\vx * wx + p\vz * wz)
		If fehlt > 0
			schub# = a * wunsch * TAKT
			If schub > fehlt Then schub = fehlt
			p\vx = p\vx + schub * wx
			p\vz = p\vz + schub * wz
		EndIf
	EndIf

	If springen And p\boden
		p\vy = SP_SPRUNG
		p\boden = False
	EndIf
	If p\boden Then p\vy = 0 Else p\vy = p\vy - SP_SCHWERE * TAKT

	TranslateEntity p\koerper, p\vx * TAKT, p\vy * TAKT, p\vz * TAKT
End Function

Function Spieler_Nach(p.Spieler)
	p\boden = False
	For i = 1 To CountCollisions(p\koerper)
		nx# = CollisionNX(p\koerper, i)
		ny# = CollisionNY(p\koerper, i)
		nz# = CollisionNZ(p\koerper, i)
		If ny > SP_BODEN_NY Then p\boden = True
		; Den Teil der Geschwindigkeit, der in die Flaeche zeigt, abschneiden.
		d# = p\vx * nx + p\vy * ny + p\vz * nz
		If d < 0
			p\vx = p\vx - nx * d
			p\vy = p\vy - ny * d
			p\vz = p\vz - nz * d
		EndIf
	Next
	; Bodenpruefung - nicht beim Aufsteigen, sonst endet jeder Sprung sofort.
	If p\vy <= 0
		k = p\koerper
		unten# = EntityY(k) - SP_HALB + SP_FUSS_R
		If LinePick(EntityX(k), unten, EntityZ(k), 0, -SP_SCHNAPP, 0, SP_FUSS_R)
			If PickedNY() > SP_BODEN_NY
				p\boden = True
				p\vy = 0
				luecke# = SP_SCHNAPP * PickedTime()
				If luecke > 0.002 Then TranslateEntity k, 0, -(luecke - 0.001), 0
			EndIf
		EndIf
	EndIf
End Function

Function Spieler_Tempo#(p.Spieler)
	Return Sqr(p\vx * p\vx + p\vz * p\vz)
End Function
