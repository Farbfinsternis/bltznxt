; Leuchtturm - die Waffen (Schritt 4).
;
; Drei Waffen, drei Techniken (LEUCHTTURM.md):
;
;   1 Maschinengewehr   Sofort-Treffer per LinePick, mit Streuung
;   2 Raketenwerfer     Geschoss mit Kollision, Explosion mit Flaechenschaden
;                       und Rueckstoss - daraus wird der Rocket-Jump
;   3 Railgun           Sofort-Treffer per LinePick, sichtbare Spur
;
; Die Werte sind die von Quake III, umgerechnet wie die Bewegung (eine
; Quake-Einheit = 3.2 cm, spieler.bb):
;
;                  Takt     Schaden          sonst
;   MG             0.1 s    7                Streuung 200/8192 (1.4 Grad)
;   Raketenwerfer  0.8 s    100 direkt,      28.9 m/s (900 u/s),
;                           100 in 3.85 m    Radius 120 u
;   Railgun        1.5 s    100
;
; Flaechenschaden faellt mit dem Abstand zur naechsten Stelle des Getroffenen
; linear auf null. Der Rueckstoss ist der von Quake: 1000 * Schaden / 200
; u/s, also 0.16 m/s je Schadenspunkt, gerichtet von der Explosion zur
; Mitte des Spielers, 24 u (0.77 m) nach oben verschoben. Eigenschaden gibt
; es noch nicht (Gesundheit kommt mit Schritt 7), der Rueckstoss wirkt aber
; schon - wer vor die Fuesse schiesst und springt, fliegt.
;
; Wechsel wie in Quake: erst wenn die Waffe wieder feuerbereit ist, dann
; 0.2 s senken, 0.25 s heben. Munition: bis es Items gibt (Schritt 7), hat
; jede Waffe die Hoechstmenge aus Quake, 200 Schuss.
;
; Die Modelle (daten/mg.glb, rl.glb, rail.glb, bis Schritt 5 ohne Animation)
; haengen an der Kamera; ihr Kind "muendung" ist der Ort von Muendungsfeuer,
; Railspur und - wie in Quake nicht dort, sondern auf der Blicklinie - der
; Start der Rakete.
;
;   Waffen_Laden p, ordner$
;   Waffe_Waehlen nr   /   Waffe_Blaettern richtung       ; bei Eingabe
;   Raketen_Fliegen                 ; je Takt, vor UpdateWorld
;   Raketen_Nach p                  ; je Takt, nach UpdateWorld
;   Waffen_Takt p, abzug            ; je Takt, danach

Const W_MG = 1, W_RL = 2, W_RAIL = 3
Const W_ANZAHL = 3
Const W_WEITE# = 263.0          ; 8192 u
Const W_STOSS# = 0.16           ; Rueckstoss in m/s je Schadenspunkt
Const W_SENKEN = 12, W_HEBEN = 15  ; Takte: 0.2 s und 0.25 s
Const W_VOLL = 200              ; Munition je Waffe

Const MG_SCHADEN = 7
Const MG_STREU# = 0.0244
Const RAIL_SCHADEN = 100
Const RK_TEMPO# = 28.9
Const RK_SCHADEN = 100
Const RK_FLAECHE = 100
Const RK_RADIUS# = 3.85
Const RK_LEBEN# = 15.0
Const RK_START# = 0.45          ; so weit vor dem Auge beginnt die Rakete

Dim w_name$(W_ANZAHL), w_takt(W_ANZAHL), w_munition(W_ANZAHL)
Dim w_modell(W_ANZAHL), w_muendung(W_ANZAHL), w_feuer(W_ANZAHL), w_kick#(W_ANZAHL)

Global w_aktiv, w_wunsch
Global w_warte                  ; Takte bis zum naechsten Schuss
Global w_zustand, w_zeit        ; 0 bereit, 1 senken, 2 heben; Takte uebrig
Global w_rueck#                 ; Rueckstoss des Modells in m
Global w_feuer_zeit             ; Takte Muendungsfeuer
Global w_bob#
Global w_ex#, w_ey#, w_ez#      ; Endpunkt des letzten Strahls
Global w_schuesse, w_explosionen

Type Rakete
	Field ent
	Field vx#, vy#, vz#
	Field alter#
	Field takte
End Type

Function Waffen_Laden(p.Spieler, ordner$)
	w_name(W_MG) = "MG" : w_name(W_RL) = "Raketenwerfer" : w_name(W_RAIL) = "Railgun"
	w_takt(W_MG) = 6 : w_takt(W_RL) = 48 : w_takt(W_RAIL) = 90     ; 0.1, 0.8, 1.5 s
	w_kick(W_MG) = 0.015 : w_kick(W_RL) = 0.05 : w_kick(W_RAIL) = 0.07
	Restore Waffen_Dateien
	For i = 1 To W_ANZAHL
		Read datei$, r, g, b
		w_modell(i) = LoadAnimMesh(ordner + "/" + datei)
		If w_modell(i) = 0 Then RuntimeError "Waffe nicht lesbar: " + ordner + "/" + datei
		EntityParent w_modell(i), p\kamera, False
		w_muendung(i) = FindChild(w_modell(i), "muendung")
		If w_muendung(i) = 0 Then w_muendung(i) = w_modell(i)
		w_feuer(i) = CreateSprite(w_muendung(i))
		EntityTexture w_feuer(i), fx_glanz
		EntityBlend w_feuer(i), 3
		EntityFX w_feuer(i), 1 + 8
		EntityColor w_feuer(i), r, g, b
		ScaleSprite w_feuer(i), 0.07, 0.07
		HideEntity w_feuer(i)
		HideEntity w_modell(i)
		w_munition(i) = W_VOLL
	Next
	Collisions TYP_RAKETE, TYP_WELT, 2, 1
	Collisions TYP_RAKETE, TYP_ZIEL, 2, 1
	w_aktiv = W_MG : w_wunsch = W_MG
	w_warte = 0 : w_zustand = 2 : w_zeit = W_HEBEN
	w_rueck = 0 : w_schuesse = 0
	ShowEntity w_modell(w_aktiv)
	Waffen_Zeigen(p)
End Function

.Waffen_Dateien
Data "mg.glb", 255, 220, 120
Data "rl.glb", 255, 170, 80
Data "rail.glb", 120, 220, 255

Function Waffe_Waehlen(nr)
	If nr >= 1 And nr <= W_ANZAHL
		If w_munition(nr) > 0 Then w_wunsch = nr
	EndIf
End Function

; Naechste (1) oder vorige (-1) Waffe mit Munition.
Function Waffe_Blaettern(richtung)
	nr = w_wunsch
	For i = 1 To W_ANZAHL
		nr = nr + richtung
		If nr > W_ANZAHL Then nr = 1
		If nr < 1 Then nr = W_ANZAHL
		If w_munition(nr) > 0 Then w_wunsch = nr : Return
	Next
End Function

Function Waffen_Takt(p.Spieler, abzug)
	; Wie in Quake laeuft die Wartezeit nur, solange sie positiv ist - sonst
	; saehe ein Schuss nach dem Wechsel die Zeit des Wechsels als Guthaben.
	If w_warte > 0 Then w_warte = w_warte - 1
	Select w_zustand
	Case 0
		If w_wunsch <> w_aktiv And w_warte <= 0
			w_zustand = 1 : w_zeit = W_SENKEN
		EndIf
	Case 1
		w_zeit = w_zeit - 1
		If w_zeit <= 0
			HideEntity w_modell(w_aktiv)
			HideEntity w_feuer(w_aktiv)
			w_aktiv = w_wunsch
			ShowEntity w_modell(w_aktiv)
			w_zustand = 2 : w_zeit = W_HEBEN
		EndIf
	Case 2
		w_zeit = w_zeit - 1
		If w_zeit <= 0 Then w_zustand = 0
	End Select

	If w_zustand = 0 And w_warte <= 0
		If abzug
			If w_munition(w_aktiv) > 0
				Waffe_Feuern(p)
				w_munition(w_aktiv) = w_munition(w_aktiv) - 1
				w_schuesse = w_schuesse + 1
				w_warte = w_warte + w_takt(w_aktiv)
			Else
				; Leer: kurz warten, dann zur naechsten Waffe mit Munition.
				w_warte = 30
				Waffe_Blaettern(1)
			EndIf
		Else
			w_warte = 0
		EndIf
	EndIf

	If w_feuer_zeit > 0
		w_feuer_zeit = w_feuer_zeit - 1
		If w_feuer_zeit <= 0 Then HideEntity w_feuer(w_aktiv)
	EndIf
	w_rueck = w_rueck - 0.4 * TAKT
	If w_rueck < 0 Then w_rueck = 0
	If p\boden Then w_bob = w_bob + TAKT * Spieler_Tempo(p) / SP_LAUF
	Waffen_Zeigen(p)
End Function

; Das Modell in der Hand: gesenkt beim Wechsel, Rueckstoss nach hinten,
; beim Laufen ein leichtes Wippen.
Function Waffen_Zeigen(p.Spieler)
	tief# = 0
	If w_zustand = 1 Then tief = 0.25 * (1 - Float(w_zeit) / W_SENKEN)
	If w_zustand = 2 Then tief = 0.25 * Float(w_zeit) / W_HEBEN
	st# = Spieler_Tempo(p) / SP_LAUF
	If st > 1 Then st = 1
	If Not p\boden Then st = 0
	bx# = 0.008 * st * Sin(w_bob * 360)
	by# = 0.005 * st * Abs(Cos(w_bob * 360))
	PositionEntity w_modell(w_aktiv), 0.13 + bx, -0.12 - tief + by, 0.10 - w_rueck
End Function

Function Waffe_Feuern(p.Spieler)
	k = p\kamera
	ax# = EntityX(k, True) : ay# = EntityY(k, True) : az# = EntityZ(k, True)
	m = w_muendung(w_aktiv)
	mx# = EntityX(m, True) : my# = EntityY(m, True) : mz# = EntityZ(m, True)
	ShowEntity w_feuer(w_aktiv)
	RotateSprite w_feuer(w_aktiv), Rnd(360)
	w_feuer_zeit = 3
	w_rueck = w_kick(w_aktiv)

	Select w_aktiv
	Case W_MG
		; Streuung wie in Quake: zufaellige Richtung, zufaelliger Anteil
		w# = Rnd(360)
		TFormVector Sin(w) * Rnd(-1, 1) * MG_STREU, Cos(w) * Rnd(-1, 1) * MG_STREU, 1, k, 0
		Waffe_Strahl(ax, ay, az, TFormedX(), TFormedY(), TFormedZ(), MG_SCHADEN, W_MG)
	Case W_RAIL
		TFormVector 0, 0, 1, k, 0
		Waffe_Strahl(ax, ay, az, TFormedX(), TFormedY(), TFormedZ(), RAIL_SCHADEN, W_RAIL)
		Effekt_Spur(mx, my, mz, w_ex, w_ey, w_ez, 120, 220, 255, 0.07, 0.9)
		; Wirbel entlang der Spur
		dx# = w_ex - mx : dy# = w_ey - my : dz# = w_ez - mz
		l# = Sqr(dx * dx + dy * dy + dz * dz)
		n = Int(l / 0.4)
		If n > 150 Then n = 150
		For i = 1 To n
			t# = Float(i) / n
			Effekt_Sprite(mx + dx * t, my + dy * t, mz + dz * t, fx_glanz, 3, 90, 180, 255, 0.1, 0.35, 0.7, 0, 0.8, Rnd(-0.3, 0.3), Rnd(-0.3, 0.3), Rnd(-0.3, 0.3))
		Next
	Case W_RL
		TFormVector 0, 0, 1, k, 0
		Rakete_Neu(ax, ay, az, TFormedX(), TFormedY(), TFormedZ(), p)
	End Select
End Function

; Sofort-Treffer von (x,y,z) in Richtung (dx,dy,dz). Der Endpunkt steht
; danach in w_ex, w_ey, w_ez.
Function Waffe_Strahl(x#, y#, z#, dx#, dy#, dz#, schaden, art)
	l# = Sqr(dx * dx + dy * dy + dz * dz)
	dx = dx / l : dy = dy / l : dz = dz / l
	e = LinePick(x, y, z, dx * W_WEITE, dy * W_WEITE, dz * W_WEITE)
	If e = 0
		w_ex = x + dx * W_WEITE : w_ey = y + dy * W_WEITE : w_ez = z + dz * W_WEITE
		Return
	EndIf
	w_ex = PickedX() : w_ey = PickedY() : w_ez = PickedZ()
	nx# = PickedNX() : ny# = PickedNY() : nz# = PickedNZ()
	ziel.Ziel = Ziel_Von(e)
	If ziel <> Null
		Ziel_Treffer(ziel, schaden)
		For i = 1 To 4
			Effekt_Sprite(w_ex, w_ey, w_ez, fx_glanz, 3, 255, 90, 60, 0.15, 0.03, 1, 0, 0.25, nx * 2 + Rnd(-2, 2), ny * 2 + Rnd(-1, 3), nz * 2 + Rnd(-2, 2))
		Next
		Return
	EndIf
	If art = W_MG
		Effekt_Sprite(w_ex + nx * 0.05, w_ey + ny * 0.05, w_ez + nz * 0.05, fx_glanz, 3, 255, 220, 140, 0.25, 0.05, 1, 0, 0.12)
		Effekt_Sprite(w_ex + nx * 0.1, w_ey + ny * 0.1, w_ez + nz * 0.1, fx_rauch, 1, 170, 165, 155, 0.15, 0.6, 0.6, 0, 0.6, nx * 0.4, 0.4, nz * 0.4)
		Effekt_Fleck(w_ex, w_ey, w_ez, nx, ny, nz, 0.12)
	Else
		Effekt_Sprite(w_ex + nx * 0.05, w_ey + ny * 0.05, w_ez + nz * 0.05, fx_glanz, 3, 120, 220, 255, 0.8, 0.1, 1, 0, 0.4)
		Effekt_Fleck(w_ex, w_ey, w_ez, nx, ny, nz, 0.35)
	EndIf
End Function

; Neue Rakete vom Auge (x,y,z) in Richtung (dx,dy,dz). Steht man mit der
; Nase an der Wand, explodiert sie sofort.
Function Rakete_Neu(x#, y#, z#, dx#, dy#, dz#, p.Spieler)
	e = LinePick(x, y, z, dx * (RK_START + 0.1), dy * (RK_START + 0.1), dz * (RK_START + 0.1))
	If e
		Explosion(PickedX() - dx * 0.1, PickedY() - dy * 0.1, PickedZ() - dz * 0.1, PickedNX(), PickedNY(), PickedNZ(), Ziel_Von(e), p)
		Return
	EndIf
	r.Rakete = New Rakete
	r\ent = CreatePivot()
	PositionEntity r\ent, x + dx * RK_START, y + dy * RK_START, z + dz * RK_START
	EntityRadius r\ent, 0.1
	EntityType r\ent, TYP_RAKETE
	ResetEntity r\ent
	s = CreateSprite(r\ent)
	EntityTexture s, fx_glanz
	EntityBlend s, 3
	EntityFX s, 1 + 8
	EntityColor s, 255, 190, 100
	ScaleSprite s, 0.2, 0.2
	r\vx = dx * RK_TEMPO : r\vy = dy * RK_TEMPO : r\vz = dz * RK_TEMPO
End Function

Function Raketen_Fliegen()
	For r.Rakete = Each Rakete
		TranslateEntity r\ent, r\vx * TAKT, r\vy * TAKT, r\vz * TAKT
		r\alter = r\alter + TAKT
		r\takte = r\takte + 1
		If r\takte Mod 2 = 0
			Effekt_Sprite(EntityX(r\ent), EntityY(r\ent), EntityZ(r\ent), fx_rauch, 1, 190, 190, 190, 0.15, 0.7, 0.5, 0, 0.8, 0, 0.3, 0)
		EndIf
	Next
End Function

Function Raketen_Nach(p.Spieler)
	For r.Rakete = Each Rakete
		If CountCollisions(r\ent) > 0
			Explosion(EntityX(r\ent), EntityY(r\ent), EntityZ(r\ent), CollisionNX(r\ent, 1), CollisionNY(r\ent, 1), CollisionNZ(r\ent, 1), Ziel_Von(CollisionEntity(r\ent, 1)), p)
			FreeEntity r\ent
			Delete r
		ElseIf r\alter >= RK_LEBEN
			Explosion(EntityX(r\ent), EntityY(r\ent), EntityZ(r\ent), 0, 0, 0, Null, p)
			FreeEntity r\ent
			Delete r
		EndIf
	Next
End Function

; Explosion bei (x,y,z) an einer Flaeche mit der Normalen (nx,ny,nz) - oder
; mitten in der Luft, dann ist die Normale null. `direkt` ist die Scheibe,
; die die Rakete getroffen hat: sie bekommt den vollen Schaden und keinen
; Flaechenschaden dazu.
Function Explosion(x#, y#, z#, nx#, ny#, nz#, direkt.Ziel, p.Spieler)
	w_explosionen = w_explosionen + 1
	If direkt <> Null Then Ziel_Treffer(direkt, RK_SCHADEN)
	For zl.Ziel = Each Ziel
		If zl <> direkt And zl\weg <= 0
			ex# = EntityX(zl\ent) : ey# = EntityY(zl\ent) : ez# = EntityZ(zl\ent)
			d# = Sqr((ex - x) ^ 2 + (ey - y) ^ 2 + (ez - z) ^ 2) - 0.45
			If d < 0 Then d = 0
			If d < RK_RADIUS
				If Waffe_Frei(x, y, z, ex, ey, ez) Then Ziel_Treffer(zl, Int(RK_FLAECHE * (1 - d / RK_RADIUS)))
			EndIf
		EndIf
	Next

	; Der Spieler: Abstand zur naechsten Stelle seines Koerpers (ein Quader
	; 0.8 x 1.8 x 0.8), Stoss von der Explosion weg und etwas nach oben.
	k = p\koerper
	px# = EntityX(k) : py# = EntityY(k) : pz# = EntityZ(k)
	cx# = x : If cx < px - SP_RADIUS Then cx = px - SP_RADIUS
	If cx > px + SP_RADIUS Then cx = px + SP_RADIUS
	cy# = y : If cy < py - SP_HALB Then cy = py - SP_HALB
	If cy > py + SP_HALB Then cy = py + SP_HALB
	cz# = z : If cz < pz - SP_RADIUS Then cz = pz - SP_RADIUS
	If cz > pz + SP_RADIUS Then cz = pz + SP_RADIUS
	d# = Sqr((cx - x) ^ 2 + (cy - y) ^ 2 + (cz - z) ^ 2)
	If d < RK_RADIUS
		If Waffe_Frei(x, y, z, px, py, pz)
			punkte# = RK_FLAECHE * (1 - d / RK_RADIUS)
			rx# = px - x : ry# = py - y + 0.77 : rz# = pz - z
			l# = Sqr(rx * rx + ry * ry + rz * rz)
			If l > 0
				f# = punkte * W_STOSS / l
				Spieler_Stoss(p, rx * f, ry * f, rz * f)
			EndIf
		EndIf
	EndIf

	; Zu sehen: Feuerball, Blitz, Rauch, Funken, Licht, Brandfleck
	Effekt_Sprite(x, y, z, fx_glanz, 3, 255, 150, 60, 0.6, 3.2, 1, 0, 0.45)
	Effekt_Sprite(x, y, z, fx_glanz, 3, 255, 240, 200, 0.4, 2.0, 1, 0, 0.2)
	For i = 1 To 6
		Effekt_Sprite(x + Rnd(-0.4, 0.4), y + Rnd(-0.2, 0.4), z + Rnd(-0.4, 0.4), fx_rauch, 1, 90, 85, 80, 0.8, 2.2, 0.6, 0, Rnd(0.9, 1.4), Rnd(-0.4, 0.4), Rnd(0.5, 1.2), Rnd(-0.4, 0.4))
	Next
	For i = 1 To 10
		Effekt_Sprite(x, y, z, fx_glanz, 3, 255, 200, 100, 0.12, 0.03, 1, 0, Rnd(0.3, 0.6), nx * 3 + Rnd(-5, 5), ny * 3 + Rnd(-2, 6), nz * 3 + Rnd(-5, 5))
	Next
	Effekt_Licht(x, y, z, 255, 170, 90, 9, 0.4)
	If nx <> 0 Or ny <> 0 Or nz <> 0
		Effekt_Fleck(x - nx * 0.1, y - ny * 0.1, z - nz * 0.1, nx, ny, nz, 1.3)
	EndIf
End Function

; Freie Sicht von (x,y,z) nach (x2,y2,z2)? Scheiben verdecken nichts.
Function Waffe_Frei(x#, y#, z#, x2#, y2#, z2#)
	e = LinePick(x, y, z, x2 - x, y2 - y, z2 - z)
	If e = 0 Then Return True
	If Ziel_Von(e) <> Null Then Return True
	Return False
End Function

Function Waffen_Leeren()
	For r.Rakete = Each Rakete
		FreeEntity r\ent
		Delete r
	Next
End Function
