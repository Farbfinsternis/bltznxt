; Blockwelt - Spieler
;
; Der Spieler ist ein Quader von 0.6 x 1.8 x 0.6 Bloecken, die Augen sitzen
; auf 1.62. Kollidiert wird nicht mit den Meshes, sondern direkt mit den
; Bloecken der Welt: Bewegung Achse fuer Achse, stoesst der Quader an, rueckt
; er bis an die Blockkante. Das ist schneller als Dreieckskollision und kennt
; keine Naehte zwischen Chunks.
;
; Die Welt laeuft in festen Takten zu 1/60 s; die Kamera steht zwischen dem
; letzten und dem aktuellen Takt, damit es bei hoher Bildrate nicht ruckelt.

Const TAKT# = 1.0 / 60.0
Const SP_HALB# = 0.3
Const SP_HOEHE# = 1.8
Const SP_AUGE# = 1.62
Const SP_LAUF# = 4.3
Const SP_RENN# = 5.6
Const SP_FLUG# = 11.0
Const SP_SPRUNG# = 8.7
Const SP_SCHWERE# = 28.0
Const SP_REICHWEITE# = 6.0

Global sp_x#, sp_y#, sp_z#             ; Fuesse, Mitte
Global sp_ax#, sp_ay#, sp_az#          ; Stand beim vorigen Takt
Global sp_vx#, sp_vy#, sp_vz#
Global sp_gier#, sp_neig#
Global sp_boden, sp_wasser, sp_kopf_nass, sp_fliegen, sp_wand
Global sp_kamera, sp_nass_schleier

; Ergebnis des Blickstrahls
Global ziel_treffer, ziel_x, ziel_y, ziel_z, ziel_vx, ziel_vy, ziel_vz

Function Spieler_Neu(x#, y#, z#)
	sp_kamera = CreateCamera()
	CameraRange sp_kamera, 0.05, (SICHT + 1) * CH * 1.25
	CameraClsColor sp_kamera, 150, 196, 255
	; blauer Schleier vor der Linse, wenn der Kopf unter Wasser ist
	sp_nass_schleier = CreateSprite(sp_kamera)
	PositionEntity sp_nass_schleier, 0, 0, 0.1
	ScaleSprite sp_nass_schleier, 0.4, 0.4
	EntityColor sp_nass_schleier, 30, 70, 170
	EntityAlpha sp_nass_schleier, 0.55
	EntityFX sp_nass_schleier, 1
	EntityOrder sp_nass_schleier, -10
	HideEntity sp_nass_schleier
	Spieler_Setzen(x, y, z)
End Function

Function Spieler_Setzen(x#, y#, z#)
	sp_x = x : sp_y = y : sp_z = z
	sp_ax = x : sp_ay = y : sp_az = z
	sp_vx = 0 : sp_vy = 0 : sp_vz = 0
End Function

Function Spieler_Schauen(dg#, dn#)
	sp_gier = sp_gier + dg
	sp_neig = sp_neig + dn
	If sp_neig > 89.5 Then sp_neig = 89.5
	If sp_neig < -89.5 Then sp_neig = -89.5
End Function

; Ueberschneidet der Quader an dieser Stelle einen festen Block?
Function Box_Stoesst(x#, y#, z#)
	x0 = Floor(x - SP_HALB) : x1 = Ceil(x + SP_HALB) - 1
	y0 = Floor(y) : y1 = Ceil(y + SP_HOEHE) - 1
	z0 = Floor(z - SP_HALB) : z1 = Ceil(z + SP_HALB) - 1
	For by = y0 To y1
		For bz = z0 To z1
			For bx = x0 To x1
				If bl_fest(Welt_Block(bx, by, bz)) Then Return True
			Next
		Next
	Next
	Return False
End Function

Function Box_Im_Wasser(x#, y#, z#, h#)
	x0 = Floor(x - SP_HALB) : x1 = Ceil(x + SP_HALB) - 1
	y0 = Floor(y) : y1 = Ceil(y + h) - 1
	z0 = Floor(z - SP_HALB) : z1 = Ceil(z + SP_HALB) - 1
	For by = y0 To y1
		For bz = z0 To z1
			For bx = x0 To x1
				If Welt_Block(bx, by, bz) = B_WASSER Then Return True
			Next
		Next
	Next
	Return False
End Function

; vor, seit: -1..1; hoch: Leertaste gehalten; runter: Umschalt; rennen: Strg
Function Spieler_Takt(vor#, seit#, hoch, runter, rennen)
	Local tempo#, folgen#, zy#
	sp_ax = sp_x : sp_ay = sp_y : sp_az = sp_z

	sp_wasser = Box_Im_Wasser(sp_x, sp_y, sp_z, 1.0)
	sp_kopf_nass = (Welt_Block(Floor(sp_x), Floor(sp_y + SP_AUGE), Floor(sp_z)) = B_WASSER)

	; Wunschrichtung in der Ebene
	sg# = Sin(sp_gier) : cg# = Cos(sp_gier)
	wx# = -sg * vor + cg * seit
	wz# = cg * vor + sg * seit
	l# = Sqr(wx * wx + wz * wz)
	If l > 1 Then wx = wx / l : wz = wz / l

	If sp_fliegen
		tempo# = SP_FLUG
		If rennen Then tempo = SP_FLUG * 2.5
		folgen# = 0.25
		zy# = 0
		If hoch Then zy = tempo
		If runter Then zy = -tempo
		sp_vy = sp_vy + (zy - sp_vy) * folgen
	ElseIf sp_wasser
		tempo = SP_LAUF * 0.55
		folgen = 0.12
		sp_vy = sp_vy * 0.9 - 9.0 * TAKT
		If hoch
			sp_vy = sp_vy + 22.0 * TAKT
			If sp_vy > 3.2 Then sp_vy = 3.2
			; an einer Kante: hinausspringen
			If sp_wand And (Not sp_kopf_nass) Then sp_vy = 6.5
		EndIf
		If sp_vy < -3 Then sp_vy = -3
	Else
		tempo = SP_LAUF
		If rennen Then tempo = SP_RENN
		If sp_boden Then folgen = 0.35 Else folgen = 0.06
		If hoch And sp_boden Then sp_vy = SP_SPRUNG
		sp_vy = sp_vy - SP_SCHWERE * TAKT
		If sp_vy < -50 Then sp_vy = -50
	EndIf
	sp_vx = sp_vx + (wx * tempo - sp_vx) * folgen
	sp_vz = sp_vz + (wz * tempo - sp_vz) * folgen

	; Bewegen, Achse fuer Achse
	sp_wand = False
	dx# = sp_vx * TAKT
	If Box_Stoesst(sp_x + dx, sp_y, sp_z)
		If dx > 0
			sp_x = Floor(sp_x + SP_HALB + dx) - SP_HALB - 0.001
		Else
			sp_x = Floor(sp_x - SP_HALB + dx) + 1 + SP_HALB + 0.001
		EndIf
		sp_vx = 0 : sp_wand = True
	Else
		sp_x = sp_x + dx
	EndIf

	dz# = sp_vz * TAKT
	If Box_Stoesst(sp_x, sp_y, sp_z + dz)
		If dz > 0
			sp_z = Floor(sp_z + SP_HALB + dz) - SP_HALB - 0.001
		Else
			sp_z = Floor(sp_z - SP_HALB + dz) + 1 + SP_HALB + 0.001
		EndIf
		sp_vz = 0 : sp_wand = True
	Else
		sp_z = sp_z + dz
	EndIf

	dy# = sp_vy * TAKT
	sp_boden = False
	If Box_Stoesst(sp_x, sp_y + dy, sp_z)
		If dy < 0
			sp_y = Floor(sp_y + dy) + 1
			sp_boden = True
		Else
			sp_y = Floor(sp_y + SP_HOEHE + dy) - SP_HOEHE - 0.001
		EndIf
		sp_vy = 0
	Else
		sp_y = sp_y + dy
	EndIf
End Function

; Kamera zwischen vorigem und aktuellem Takt; anteil 0..1
Function Spieler_Kamera(anteil#)
	x# = sp_ax + (sp_x - sp_ax) * anteil
	y# = sp_ay + (sp_y - sp_ay) * anteil
	z# = sp_az + (sp_z - sp_az) * anteil
	PositionEntity sp_kamera, x, y + SP_AUGE, z
	RotateEntity sp_kamera, sp_neig, sp_gier, 0
	nass = (Welt_Block(Floor(x), Floor(y + SP_AUGE), Floor(z)) = B_WASSER)
	If nass
		ShowEntity sp_nass_schleier
		CameraClsColor sp_kamera, 30, 60, 140
	Else
		HideEntity sp_nass_schleier
		CameraClsColor sp_kamera, 150, 196, 255
	EndIf
End Function

; Blickstrahl durch das Blockgitter (Amanatides & Woo). Setzt ziel_* auf den
; ersten festen Block und die Zelle davor (dort entsteht ein neuer Block).
Function Blick_Strahl()
	Local tdx#, tdy#, tdz#, tmx#, tmy#, tmz#
	ox# = EntityX(sp_kamera) : oy# = EntityY(sp_kamera) : oz# = EntityZ(sp_kamera)
	TFormVector 0, 0, 1, sp_kamera, 0
	dx# = TFormedX() : dy# = TFormedY() : dz# = TFormedZ()

	x = Floor(ox) : y = Floor(oy) : z = Floor(oz)
	ziel_treffer = False
	If dx > 0 Then sx = 1 Else sx = -1
	If dy > 0 Then sy = 1 Else sy = -1
	If dz > 0 Then sz = 1 Else sz = -1
	If Abs(dx) > 0.00001 Then tdx# = Abs(1.0 / dx) Else tdx = 1000000
	If Abs(dy) > 0.00001 Then tdy# = Abs(1.0 / dy) Else tdy = 1000000
	If Abs(dz) > 0.00001 Then tdz# = Abs(1.0 / dz) Else tdz = 1000000
	If sx > 0 Then tmx# = (x + 1 - ox) * tdx Else tmx = (ox - x) * tdx
	If sy > 0 Then tmy# = (y + 1 - oy) * tdy Else tmy = (oy - y) * tdy
	If sz > 0 Then tmz# = (z + 1 - oz) * tdz Else tmz = (oz - z) * tdz

	vx = x : vy = y : vz = z
	t# = 0
	While t <= SP_REICHWEITE
		If bl_fest(Welt_Block(x, y, z))
			ziel_treffer = True
			ziel_x = x : ziel_y = y : ziel_z = z
			ziel_vx = vx : ziel_vy = vy : ziel_vz = vz
			Return True
		EndIf
		vx = x : vy = y : vz = z
		If tmx < tmy And tmx < tmz
			x = x + sx : t = tmx : tmx = tmx + tdx
		ElseIf tmy < tmz
			y = y + sy : t = tmy : tmy = tmy + tdy
		Else
			z = z + sz : t = tmz : tmz = tmz + tdz
		EndIf
	Wend
	Return False
End Function

; Darf an dieser Stelle ein Block entstehen, ohne den Spieler einzusperren?
Function Platz_Frei(x, y, z)
	If x + 1 <= sp_x - SP_HALB Or x >= sp_x + SP_HALB Then Return True
	If z + 1 <= sp_z - SP_HALB Or z >= sp_z + SP_HALB Then Return True
	If y + 1 <= sp_y Or y >= sp_y + SP_HOEHE Then Return True
	Return False
End Function
