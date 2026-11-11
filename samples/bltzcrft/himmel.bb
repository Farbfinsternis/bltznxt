; BLTZCRFT - Himmel: Tag und Nacht, Sonne, Mond, Sterne und Wolken
;
; Ein Tag dauert wie in Minecraft 20 Minuten. Die Sonne geht um 6 Uhr im
; Osten (+x) auf, steht um 12 Uhr im Zenit und geht um 18 Uhr im Westen
; unter; der Mond steht ihr gegenueber und wechselt jede Nacht die Phase.
;
; Licht gibt es in der Welt keines: die Bloecke tragen ihre Helligkeit in den
; Vertexfarben, ohne volle Helligkeit (bloecke.bb). Ihre Farbe ist darum
; Vertexfarbe mal Umgebungslicht - AmbientLight macht die ganze Welt mit
; einem Befehl hell oder dunkel, ohne einen Chunk neu zu bauen.
;
; Der Himmel ist eine Kuppel um die Kamera, ihre Vertexfarben laufen vom
; Horizont (= Nebelfarbe, damit das Gelaende nahtlos im Dunst verschwindet)
; zum Zenit; bei Sonnenauf- und -untergang gluehen sie zur Sonne hin orange.
; Sonne, Mond und Sterne haengen an einem Drehpunkt, der sich mit der
; Tageszeit um die Ost-West-Achse dreht. Alles Himmlische hat eine
; EntityOrder > 0: es wird zuerst und ohne Z-Puffer gezeichnet, die Welt
; liegt immer davor - wie weit die Kuppel wirklich weg ist, spielt keine
; Rolle.
;
; Die Wolken sind eine flache Decke in WOLKEN_Y, hoeher als jeder Block,
; die langsam nach Osten zieht. Sie reicht weiter als die Kamera sieht; damit
; sie trotzdem ganz ins Bild passt, wird sie um die Kamera herum verkleinert
; und naeher gerueckt (eine Streckung mit der Kamera als Zentrum aendert das
; Bild nicht, nur die Tiefe). Weil das Gelaende unter der Decke endet,
; reicht die Zeichenreihenfolge: von unten liegt die Welt davor (Order 1),
; von oben dahinter (Order -1). Nur ein selbst gebauter Turm ueber die Decke
; hinaus (die Welt reicht bis 255) stuende von unten hinter den Wolken.

Const TAG_MS = 1200000          ; ein Tag: 20 Minuten
Const RAFFER_FAKTOR = 60            ; F4: Zeitraffer, ein Tag in 20 s
Const HM_RINGE = 12             ; Ringe der Kuppel (13 Hoehen, s. Data)
Const HM_SEGMENTE = 24          ; Segmente rundherum
Const KUPPEL_RADIUS# = 30.0         ; Radius der Kuppel
Const STERNE_ANZAHL = 400
Const WOLKEN_Y# = 204.0         ; Hoehe der Wolkendecke, ueber jedem Gebirge
Const WOLKEN_ZELLE = 12         ; Bloecke je Wolkenzelle
Const WOLKEN_MUSTER = 64        ; Zellen je Kante, dann wiederholt es sich
Const WOLKEN_PX = 8             ; Bildpunkte je Zelle in der Textur
Const WOLKEN_WEIT# = 420.0      ; halbe Kantenlaenge der Decke
Const WOLKEN_GITTER = 24        ; Vierecke je Kante
Const WOLKEN_TEMPO# = 1.5       ; Bloecke je Sekunde nach Osten

Global hm_ms#                   ; Millisekunden seit Mitternacht
Global hm_tag                   ; der wievielte Tag (fuer Mondphase und Wolken)
Global hm_raffer = False
Global hm_stunde#               ; 0..24, fuer die Anzeige
Global hm_himmel, hm_kuppel, hm_kuppel_s, hm_drehung, hm_dreh_vz#
Global hm_sonne, hm_mond, hm_mond_phase = -1, hm_sterne
Global hm_wolken, hm_wolken_s, hm_wolken_tex

Dim hm_ring_e#(HM_RINGE)
Dim hm_ke#((HM_RINGE + 1) * HM_SEGMENTE)       ; Sinus der Hoehe je Vertex
Dim hm_kx#((HM_RINGE + 1) * HM_SEGMENTE)       ; Richtung nach Osten (-1..1)
Dim hm_mond_tex(7)
Dim hm_wx#((WOLKEN_GITTER + 1) * (WOLKEN_GITTER + 1))
Dim hm_wz#((WOLKEN_GITTER + 1) * (WOLKEN_GITTER + 1))
Dim hm_zelle(WOLKEN_MUSTER - 1, WOLKEN_MUSTER - 1)
Dim hm_zelle2(WOLKEN_MUSTER - 1, WOLKEN_MUSTER - 1)

; Hoehe der Ringe in Grad, am Horizont dicht, damit der Farbverlauf dort fein
; ist
.hm_ringe
Data -30, -10, -4, 0, 3, 7, 12, 18, 26, 36, 50, 68, 90

; stunde: Uhrzeit beim Start (0..24)
Function Himmel_Laden(stunde#)
	hm_ms = stunde * TAG_MS / 24.0
	hm_tag = 0

	hm_himmel = CreatePivot()
	Kuppel_Bauen()
	hm_drehung = CreatePivot(hm_himmel)

	; In welche Richtung dreht ein positiver Rollwinkel? Einmal nachsehen,
	; statt es anzunehmen: die Sonne soll von Osten (+x) nach oben steigen.
	RotateEntity hm_drehung, 0, 0, 90
	TFormPoint 1, 0, 0, hm_drehung, 0
	If TFormedY() > 0 Then hm_dreh_vz = 1 Else hm_dreh_vz = -1
	RotateEntity hm_drehung, 0, 0, 0

	hm_sonne = CreateSprite(hm_drehung)
	EntityTexture hm_sonne, Sonne_Malen()
	ScaleSprite hm_sonne, 4.5, 4.5
	PositionEntity hm_sonne, KUPPEL_RADIUS * 0.8, 0, 0
	Himmels_Koerper(hm_sonne)

	For p = 0 To 7
		hm_mond_tex(p) = Mond_Malen(p)
	Next
	hm_mond = CreateSprite(hm_drehung)
	ScaleSprite hm_mond, 3.2, 3.2
	PositionEntity hm_mond, -KUPPEL_RADIUS * 0.8, 0, 0
	Himmels_Koerper(hm_mond)

	Sterne_Bauen()
	Wolken_Bauen()
	Himmel_Takt(0)
End Function

; Sonne und Mond: leuchten selbst, kein Nebel, addiert auf den Himmel -
; Schwarz in der Textur ist durchsichtig, ein Rand darf weich glimmen.
Function Himmels_Koerper(s)
	SpriteViewMode s, 1
	EntityFX s, 1 + 8
	EntityBlend s, 3
	EntityOrder s, 2
End Function

Function Kuppel_Bauen()
	Restore hm_ringe
	For i = 0 To HM_RINGE
		Read hm_ring_e(i)
	Next
	hm_kuppel = CreateMesh(hm_himmel)
	hm_kuppel_s = CreateSurface(hm_kuppel)
	For i = 0 To HM_RINGE
		e# = hm_ring_e(i)
		For j = 0 To HM_SEGMENTE - 1
			az# = j * 360.0 / HM_SEGMENTE
			v = AddVertex(hm_kuppel_s, KUPPEL_RADIUS * Cos(e) * Cos(az), KUPPEL_RADIUS * Sin(e), KUPPEL_RADIUS * Cos(e) * Sin(az))
			hm_ke(v) = Sin(e)
			hm_kx(v) = Cos(az)
		Next
	Next
	For i = 0 To HM_RINGE - 1
		For j = 0 To HM_SEGMENTE - 1
			a = i * HM_SEGMENTE + j
			b = i * HM_SEGMENTE + (j + 1) Mod HM_SEGMENTE
			AddTriangle hm_kuppel_s, a, a + HM_SEGMENTE, b
			AddTriangle hm_kuppel_s, b, a + HM_SEGMENTE, b + HM_SEGMENTE
		Next
	Next
	; volle Helligkeit, Vertexfarben, kein Nebel, beide Seiten
	EntityFX hm_kuppel, 1 + 2 + 8 + 16
	EntityOrder hm_kuppel, 3
End Function

; Kleine Vierecke auf einer Kugel, alle zur Mitte gewandt. Sie drehen sich
; mit der Sonne und blenden in der Daemmerung ein und aus.
Function Sterne_Bauen()
	SeedRnd 1977
	hm_sterne = CreateMesh(hm_drehung)
	s = CreateSurface(hm_sterne)
	r# = KUPPEL_RADIUS * 0.9
	For i = 1 To STERNE_ANZAHL
		dy# = Rnd(-1, 1) : w# = Rnd(360)
		q# = Sqr(1 - dy * dy)
		dx# = q * Cos(w) : dz# = q * Sin(w)
		; zwei Achsen quer zur Richtung
		If Abs(dy) < 0.9
			ux# = -dz : uy# = 0 : uz# = dx
		Else
			ux = 0 : uy = dz : uz = -dy
		EndIf
		l# = Sqr(ux * ux + uy * uy + uz * uz)
		ux = ux / l : uy = uy / l : uz = uz / l
		wx# = dy * uz - dz * uy : wy# = dz * ux - dx * uz : wz# = dx * uy - dy * ux
		g# = Rnd(0.04, 0.09)
		If Rnd(1) < 0.08 Then g = g * 1.7
		hell = Rand(150, 255)
		rr = hell : gg = hell : bb = hell
		Select Rand(5)
		Case 1 : bb = bb * 0.8           ; gelblich
		Case 2 : rr = rr * 0.8           ; blaeulich
		End Select
		cx# = dx * r : cy# = dy * r : cz# = dz * r
		v0 = AddVertex(s, cx - ux * g - wx * g, cy - uy * g - wy * g, cz - uz * g - wz * g)
		v1 = AddVertex(s, cx + ux * g - wx * g, cy + uy * g - wy * g, cz + uz * g - wz * g)
		v2 = AddVertex(s, cx + ux * g + wx * g, cy + uy * g + wy * g, cz + uz * g + wz * g)
		v3 = AddVertex(s, cx - ux * g + wx * g, cy - uy * g + wy * g, cz - uz * g + wz * g)
		For v = v0 To v3
			VertexColor s, v, rr, gg, bb
		Next
		AddTriangle s, v0, v1, v2
		AddTriangle s, v0, v2, v3
	Next
	EntityFX hm_sterne, 1 + 2 + 8 + 16
	EntityBlend hm_sterne, 3
	EntityOrder hm_sterne, 2
End Function

; Sonne: ein helles Quadrat mit weichem, gelbem Schein drumherum.
Function Sonne_Malen()
	g = 64
	tex = CreateTexture(g, g, 1 + 8)
	puffer = TextureBuffer(tex)
	LockBuffer puffer
	For y = 0 To g - 1
		For x = 0 To g - 1
			dx# = Abs(x - 31.5) : dy# = Abs(y - 31.5)
			If dx < 10 And dy < 10
				r = 255 : gg = 250 : b = 215
			Else
				; Schein: faellt mit dem Abstand vom Quadrat ab
				ax# = dx - 10 : If ax < 0 Then ax = 0
				ay# = dy - 10 : If ay < 0 Then ay = 0
				f# = 1 - Sqr(ax * ax + ay * ay) / 22.0
				If f < 0 Then f = 0
				f = f * f * 0.75
				r = 255 * f : gg = 200 * f : b = 110 * f
			EndIf
			WritePixelFast x, y, Rgb(r, gg, b), puffer
		Next
	Next
	UnlockBuffer puffer
	Return tex
End Function

; Mond in Phase p: 0 Vollmond, 4 Neumond. Ein Quadrat aus 12 x 12 groben
; Bildpunkten mit Kratern; der Schatten waechst von rechts und weicht nach
; links zurueck.
Function Mond_Malen(p)
	SeedRnd 42                     ; dieselben Krater in jeder Phase
	z = 4                          ; Bildpunkte je grobem Bildpunkt
	g = 16 * z
	tex = CreateTexture(g, g, 1 + 8)
	puffer = TextureBuffer(tex)
	LockBuffer puffer
	For y = 0 To g - 1
		For x = 0 To g - 1
			WritePixelFast x, y, 0, puffer
		Next
	Next
	For y = 0 To 11
		For x = 0 To 11
			h = Rand(195, 225)
			If Rand(6) = 1 Then h = Rand(140, 165)       ; Krater
			; Schatten der Phase: Spalten 0..11
			dunkel = False
			If p >= 1 And p <= 3 And x >= 12 - p * 3 Then dunkel = True
			If p = 4 Then dunkel = True
			If p >= 5 And x < (8 - p) * 3 Then dunkel = True
			If dunkel Then h = h * 0.07
			farbe = Rgb(h * 0.95, h * 0.97, h)
			For yy = 0 To z - 1
				For xx = 0 To z - 1
					WritePixelFast (x + 2) * z + xx, (y + 2) * z + yy, farbe, puffer
				Next
			Next
		Next
	Next
	UnlockBuffer puffer
	Return tex
End Function

; Wolkenmuster: Zufall auf einem Ring-Gitter, dreimal mit der Mehrheit der
; Nachbarn geglaettet - das gibt zusammenhaengende Felder statt Rauschen.
Function Wolken_Muster()
	SeedRnd 2718
	n = WOLKEN_MUSTER
	For z = 0 To n - 1
		For x = 0 To n - 1
			hm_zelle(x, z) = (Rnd(1) < 0.4)
		Next
	Next
	For runde = 1 To 3
		For z = 0 To n - 1
			For x = 0 To n - 1
				c = 0
				For dz = -1 To 1
					For dx = -1 To 1
						c = c + hm_zelle((x + dx + n) Mod n, (z + dz + n) Mod n)
					Next
				Next
				hm_zelle2(x, z) = (c >= 5)
			Next
		Next
		For z = 0 To n - 1
			For x = 0 To n - 1
				hm_zelle(x, z) = hm_zelle2(x, z)
			Next
		Next
	Next
End Function

Function Wolken_Bauen()
	Wolken_Muster()
	g = WOLKEN_MUSTER * WOLKEN_PX
	hm_wolken_tex = CreateTexture(g, g, 1 + 2 + 8)
	puffer = TextureBuffer(hm_wolken_tex)
	LockBuffer puffer
	For y = 0 To g - 1
		For x = 0 To g - 1
			If hm_zelle(x / WOLKEN_PX, y / WOLKEN_PX)
				WritePixelFast x, y, $FFFFFFFF, puffer
			Else
				WritePixelFast x, y, $00FFFFFF, puffer
			EndIf
		Next
	Next
	UnlockBuffer puffer

	hm_wolken = CreateMesh()
	hm_wolken_s = CreateSurface(hm_wolken)
	n = WOLKEN_GITTER
	For i = 0 To n
		For j = 0 To n
			lx# = (j * 2.0 / n - 1) * WOLKEN_WEIT
			lz# = (i * 2.0 / n - 1) * WOLKEN_WEIT
			v = AddVertex(hm_wolken_s, lx, 0, lz)
			hm_wx(v) = lx : hm_wz(v) = lz
			; nach aussen ausblenden, statt an der Kante abzubrechen
			r# = Sqr(lx * lx + lz * lz) / WOLKEN_WEIT
			al# = Klemmen((1 - r) / 0.55, 0, 1)
			VertexColor hm_wolken_s, v, 255, 255, 255, al * al
		Next
	Next
	For i = 0 To n - 1
		For j = 0 To n - 1
			a = i * (n + 1) + j
			AddTriangle hm_wolken_s, a, a + n + 1, a + 1
			AddTriangle hm_wolken_s, a + 1, a + n + 1, a + n + 2
		Next
	Next
	EntityTexture hm_wolken, hm_wolken_tex
	; volle Helligkeit (die Farbe setzt Himmel_Takt), kein Nebel, beide
	; Seiten, Vertexalpha fuer den Rand
	EntityFX hm_wolken, 1 + 8 + 16 + 32
	EntityAlpha hm_wolken, 0.92
	EntityOrder hm_wolken, 1
End Function

; 0..1 mit weichem Ein- und Auslauf
Function Glatt#(x#)
	x = Klemmen(x, 0, 1)
	Return x * x * (3 - 2 * x)
End Function

Function Misch#(a#, b#, t#)
	Return a + (b - a) * t
End Function

; Zeit weiterlaufen lassen und Himmel, Licht und Nebelfarbe danach stellen.
; dt in Sekunden. Nach Spieler_Kamera aufrufen.
Function Himmel_Takt(dt#)
	If hm_raffer Then dt = dt * RAFFER_FAKTOR
	hm_ms = hm_ms + dt * 1000
	While hm_ms >= TAG_MS
		hm_ms = hm_ms - TAG_MS
		hm_tag = hm_tag + 1
	Wend
	hm_stunde = hm_ms * 24.0 / TAG_MS

	; Sonnenstand: Winkel ueber dem Osthorizont
	w# = (hm_stunde - 6) * 15
	sy# = Sin(w)
	; 0 Nacht .. 1 Tag: hell bis kurz nach Sonnenuntergang, ganz dunkel erst
	; gegen 19 Uhr, wie in Minecraft
	tag# = Glatt((sy + 0.25) / 0.45)
	glut# = Glatt(1 - Abs(sy + 0.03) / 0.3)              ; Morgen- und Abendrot
	If Cos(w) > 0 Then osten# = 1 Else osten = -1         ; Seite der Sonne

	; Horizont = Nebel, Zenit; Abendrot faerbt den Dunst ein wenig
	hr# = Misch(12, 150, tag) : hg# = Misch(15, 196, tag) : hb# = Misch(32, 255, tag)
	hr = Misch(hr, 245, glut * 0.4) : hg = Misch(hg, 140, glut * 0.4) : hb = Misch(hb, 90, glut * 0.4)
	zr# = Misch(2, 78, tag) : zg# = Misch(3, 128, tag) : zb# = Misch(12, 235, tag)
	sp_himmel_r = hr : sp_himmel_g = hg : sp_himmel_b = hb
	sp_hell = 0.25 + 0.75 * tag

	; Umgebungslicht: nachts bleibt ein blaeuliches Mondlicht
	ar# = Misch(72, 255, tag) : ag# = Misch(78, 255, tag) : ab# = Misch(112, 255, tag)
	ag = ag * (1 - glut * 0.08) : ab = ab * (1 - glut * 0.16)
	AmbientLight ar, ag, ab
	nz_umgebung = ar / 255.0       ; fuer Licht_Bei (netz.bb)

	; Kuppel: vom Horizont zum Zenit, zur Sonne hin gluehend
	For v = 0 To (HM_RINGE + 1) * HM_SEGMENTE - 1
		e# = hm_ke(v)
		t# = Klemmen(e / 0.7, 0, 1)
		t = t * (2 - t)
		r# = Misch(hr, zr, t) : g# = Misch(hg, zg, t) : b# = Misch(hb, zb, t)
		k# = hm_kx(v) * osten
		If k > 0 And glut > 0
			k = k * k * k * glut * Klemmen(1 - e / 0.45, 0, 1) * Klemmen((e + 0.12) / 0.1, 0, 1) * 0.8
			r = Misch(r, 255, k) : g = Misch(g, 125, k) : b = Misch(b, 60, k)
		EndIf
		VertexColor hm_kuppel_s, v, r, g, b
	Next

	; Sonne, Mond, Sterne
	RotateEntity hm_drehung, 0, 0, w * hm_dreh_vz
	Himmel_Alpha(hm_sonne, Klemmen((sy + 0.12) / 0.15, 0, 1))
	Himmel_Alpha(hm_mond, Klemmen((0.12 - sy) / 0.15, 0, 1))
	st# = Klemmen(1 - tag * 1.5, 0, 1)
	Himmel_Alpha(hm_sterne, st * st * 0.9)
	; neue Phase ab Mittag, dann sieht man den Wechsel nicht
	p = (hm_tag + (hm_stunde >= 12)) Mod 8
	If p <> hm_mond_phase
		hm_mond_phase = p
		EntityTexture hm_mond, hm_mond_tex(p)
	EndIf

	; Wolken: Farbe nach der Tageszeit, morgens und abends von der tief
	; stehenden Sonne rosig angestrahlt
	wr# = Misch(34, 255, tag) : wg# = Misch(37, 255, tag) : wb# = Misch(54, 255, tag)
	wr = Misch(wr, 250, glut * 0.6) : wg = Misch(wg, 165, glut * 0.5) : wb = Misch(wb, 140, glut * 0.45)
	EntityColor hm_wolken, wr, wg, wb

	; unter Wasser und tief in einer Hoehle sieht man den Himmel nicht -
	; durch ein grosses Gewoelbe schiene er sonst hinter dem Dunst durch
	If sp_nass Or sp_hoehle > 0.6
		HideEntity hm_himmel
		HideEntity hm_wolken
		Return
	EndIf
	ShowEntity hm_himmel
	ShowEntity hm_wolken
	kx# = EntityX(sp_kamera) : ky# = EntityY(sp_kamera) : kz# = EntityZ(sp_kamera)
	PositionEntity hm_himmel, kx, ky, kz
	Wolken_Stellen(kx, ky, kz)
End Function

Function Himmel_Alpha(e, a#)
	If a <= 0.001
		HideEntity e
	Else
		ShowEntity e
		EntityAlpha e, a
	EndIf
End Function

; Die Decke folgt der Kamera, das Muster bleibt ueber der Welt stehen und
; zieht nur mit dem Wind.
Function Wolken_Stellen(kx#, ky#, kz#)
	muster# = WOLKEN_MUSTER * WOLKEN_ZELLE
	sek# = hm_tag * (TAG_MS / 1000) + hm_ms / 1000.0
	wind# = (sek * WOLKEN_TEMPO) Mod muster
	u0# = ((kx - wind) Mod muster) / muster
	v0# = (kz Mod muster) / muster
	For v = 0 To (WOLKEN_GITTER + 1) * (WOLKEN_GITTER + 1) - 1
		VertexTexCoords hm_wolken_s, v, u0 + hm_wx(v) / muster, v0 + hm_wz(v) / muster
	Next

	; verkleinern und naeher ruecken, bis der fernste Punkt vor der hinteren
	; Grenze der Kamera liegt (Spieler_Sicht)
	dh# = WOLKEN_Y - ky
	k# = (sp_nebel_fern * 0.9) / Sqr(2 * WOLKEN_WEIT * WOLKEN_WEIT + dh * dh)
	If k > 1 Then k = 1
	ScaleEntity hm_wolken, k, k, k
	PositionEntity hm_wolken, kx, ky + dh * k, kz
	If dh > 0 Then EntityOrder hm_wolken, 1 Else EntityOrder hm_wolken, -1
End Function

; Uhrzeit fuer die Anzeige, "07:05"
Function Himmel_Uhr$()
	m = Int(hm_stunde * 60) Mod 1440
	h$ = Str(m / 60) : If Len(h) < 2 Then h = "0" + h
	mm$ = Str(m Mod 60) : If Len(mm) < 2 Then mm = "0" + mm
	Return h + ":" + mm
End Function
