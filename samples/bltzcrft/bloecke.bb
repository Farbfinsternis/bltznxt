; BLTZCRFT - Bloecke
;
; Jede Blockart hat eine Nummer (ein Byte in der Welt), drei Texturen (oben,
; Seite, unten) und zwei Eigenschaften: deckend (verdeckt die Flaechen der
; Nachbarn und wirft Umgebungsschatten) und fest (man stoesst sich daran).
; Dazu wie in Minecraft die Haerte und ob die blosse Hand genuegt (abbau.bb).
; Pflanzen (Gras, Blumen, Schilf) sind weder deckend noch fest; sie stehen
; als zwei gekreuzte Flaechen in ihrer Zelle (netz.bb), ihre Texturen haben
; durchsichtige Stellen (Maske). Fackeln ebenso, am Boden aufrecht oder
; schraeg an einer Wand; sie leuchten (welt.bb, netz.bb).
;
; Jede Blocktextur liegt im Pinsel hinter einer zweiten Lage, der Lichtrampe:
; einem waagerechten Verlauf von Schwarz zu warmem Weiss, additiv
; (TextureBlend 3) und ueber den zweiten Satz Texturkoordinaten. Die
; Vertexfarbe traegt das Himmelslicht, das die Tageszeit ueber das
; Umgebungslicht dimmt; die zweite Koordinate sagt, wie viel Fackellicht
; dazukommt - unabhaengig von der Tageszeit. Erst die Summe wird mit der
; Blocktextur multipliziert, wie bei einer Lichtkarte im Original.
;
; Die Texturen malt das Programm selbst: 16 x 16 Bildpunkte Pixelkunst,
; achtfach vergroessert, damit die lineare Filterung die Kanten kaum
; verwischt. Jede Textur ist eine eigene - ein Atlas wuerde an den Raendern
; der Kacheln in die Nachbarn bluten, sobald Mipmaps im Spiel sind.

Const B_LUFT = 0
Const B_STEIN = 1
Const B_ERDE = 2
Const B_GRAS = 3
Const B_SAND = 4
Const B_WASSER = 5
Const B_KIES = 6
Const B_STAMM = 7
Const B_LAUB = 8
Const B_GRUND = 9
Const B_SCHNEE = 10
Const B_TON = 11
Const B_KOHLE = 12
Const B_EISEN = 13
Const B_SANDSTEIN = 14
Const B_HOHESGRAS = 15
Const B_LOEWENZAHN = 16
Const B_MOHN = 17
Const B_KORNBLUME = 18
Const B_SCHILF = 19
Const B_FACKEL = 20             ; am Boden
Const B_FACKEL_XP = 21          ; an der Wand: die Wand liegt bei +x
Const B_FACKEL_XM = 22          ; ... bei -x
Const B_FACKEL_ZP = 23          ; ... bei +z
Const B_FACKEL_ZM = 24          ; ... bei -z
Const B_STEINE = 25             ; lose Steine am Boden (wie eine Pflanze)
Const B_BRETTER = 26
Const B_WERKBANK = 27
Const B_STEINZIEGEL = 28
Const B_OFEN = 29
Const B_ANZAHL = 30

Const T_STEIN = 0
Const T_ERDE = 1
Const T_GRAS_OBEN = 2
Const T_GRAS_SEITE = 3
Const T_SAND = 4
Const T_WASSER = 5
Const T_KIES = 6
Const T_STAMM_SEITE = 7
Const T_STAMM_OBEN = 8
Const T_LAUB = 9
Const T_GRUND = 10
Const T_SCHNEE = 11
Const T_SCHNEE_SEITE = 12
Const T_TON = 13
Const T_KOHLE = 14
Const T_EISEN = 15
Const T_SANDSTEIN = 16
Const T_BRETTER = 17
Const T_WERKBANK_OBEN = 18
Const T_WERKBANK_SEITE = 19
Const T_STEINZIEGEL = 20
Const T_OFEN_SEITE = 21
Const T_OFEN_OBEN = 22
Const T_HOHESGRAS = 23
Const T_LOEWENZAHN = 24
Const T_MOHN = 25
Const T_KORNBLUME = 26
Const T_SCHILF = 27
Const T_FACKEL = 28
; ab hier die Gegenstaende (flach, Ding_Tex_Malen)
Const T_AST = 29
Const T_BROCKEN = 30
Const T_STEINAXT = 31
Const T_STEINHACKE = 32
Const T_STEINSCHAUFEL = 33
Const T_KOHLESTUECK = 34
Const T_EISENBARREN = 35
Const T_EISENAXT = 36
Const T_EISENHACKE = 37
Const T_EISENSCHAUFEL = 38
Const T_ANZAHL = 39
Const T_ERSTE_PFLANZE = 23      ; ab hier Pflanzentexturen (mit Maske)

Const TEX_ZOOM = 8          ; Bildpunkte je Texel

; Werkzeugarten (dinge.bb)
Const W_KEINS = 0
Const W_HACKE = 1
Const W_AXT = 2
Const W_SCHAUFEL = 3

Dim bl_tex(B_ANZAHL - 1, 2)     ; 0 oben, 1 Seite, 2 unten
Dim bl_deckend(B_ANZAHL - 1)
Dim bl_fest(B_ANZAHL - 1)
Dim bl_name$(B_ANZAHL - 1)
Dim bl_haerte#(B_ANZAHL - 1)    ; Haerte wie in Minecraft, 0 = sofort, -1 = nie
Dim bl_hand(B_ANZAHL - 1)       ; mit blosser Hand abbaubar (sonst kein Ertrag)
Dim bl_werkzeug(B_ANZAHL - 1)   ; welches Werkzeug ihn schneller abbaut (W_...)
Dim bl_stufe(B_ANZAHL - 1)      ; Werkzeugstufe fuer Ertrag: 0 Hand, 1 Stein, 2 Eisen
Dim bl_pflanze(B_ANZAHL - 1)    ; Kreuz aus zwei Flaechen statt Wuerfel
Dim bl_fackel(B_ANZAHL - 1)     ; 0 keine, 1 am Boden, 2 an einer Wand
Dim fk_wx(B_ANZAHL - 1)         ; Richtung zur Wand einer Wandfackel
Dim fk_wz(B_ANZAHL - 1)

Global licht_tex                ; die Lichtrampe (zweite Texturlage)

Dim tx_pinsel(T_ANZAHL - 1)     ; ein Brush je Textur
Dim tx_bild(T_ANZAHL - 1)

Dim tx_raster(15, 15)

Function Block_Art(nr, name$, oben, seite, unten, deckend, fest, haerte#, hand)
	bl_name(nr) = name
	bl_tex(nr, 0) = oben : bl_tex(nr, 1) = seite : bl_tex(nr, 2) = unten
	bl_deckend(nr) = deckend
	bl_fest(nr) = fest
	bl_haerte(nr) = haerte
	bl_hand(nr) = hand
	bl_werkzeug(nr) = W_KEINS
	If hand Then bl_stufe(nr) = 0 Else bl_stufe(nr) = 1
	bl_pflanze(nr) = False
	bl_fackel(nr) = 0
End Function

; Fackel: leuchtet, bricht sofort, haengt am Boden (art 1) oder an der Wand
; in Richtung wx, wz (art 2).
Function Fackel_Art(nr, name$, art, wx, wz)
	Block_Art(nr, name, T_FACKEL, T_FACKEL, T_FACKEL, False, False, 0, True)
	bl_fackel(nr) = art
	fk_wx(nr) = wx : fk_wz(nr) = wz
End Function

; Die Wandfackel, deren Wand in Richtung wx, wz liegt.
Function Fackel_An_Wand(wx, wz)
	For b = B_FACKEL_XP To B_FACKEL_ZM
		If fk_wx(b) = wx And fk_wz(b) = wz Then Return b
	Next
	Return B_FACKEL
End Function

; Pflanze: eine Textur, nicht deckend, nicht fest, bricht sofort.
Function Pflanzen_Art(nr, name$, t)
	Block_Art(nr, name, t, t, t, False, False, 0, True)
	bl_pflanze(nr) = True
End Function

Function Bloecke_Laden()
	Bloecke_Arten()

	; Die Texturen haengen nicht an der Saat der Welt.
	SeedRnd 4711
	licht_tex = Licht_Rampe()
	For t = 0 To T_ANZAHL - 1
		Tex_Malen(t)
		pflanze = (t >= T_ERSTE_PFLANZE)
		tx_bild(t) = Tex_Aus_Raster(pflanze)
		tx_pinsel(t) = CreateBrush()
		; Vertexfarben ohne volle Helligkeit: es gibt keine Lichtquelle, also
		; ist die Farbe Vertexfarbe mal Umgebungslicht - damit macht
		; himmel.bb die ganze Welt mit einem Befehl hell oder dunkel.
		If t = T_FACKEL
			; die Fackel leuchtet selbst: volle Helligkeit, beide Seiten
			BrushTexture tx_pinsel(t), tx_bild(t)
			BrushFX tx_pinsel(t), 1 + 16
		ElseIf pflanze
			; Pflanzen von beiden Seiten. Ausgeschnitten wird nur nach der
			; ersten Lage (wie im Original) - dort liegt darum eine weisse
			; Maske mit dem Alpha der Pflanze, die die Farbe nicht aendert.
			; Danach wie bei den Bloecken: Fackellicht dazu, mal Textur.
			BrushTexture tx_pinsel(t), Tex_Maske(tx_bild(t)), 0, 0
			BrushTexture tx_pinsel(t), licht_tex, 0, 1
			BrushTexture tx_pinsel(t), tx_bild(t), 0, 2
			BrushFX tx_pinsel(t), 2 + 16
		Else
			BrushTexture tx_pinsel(t), licht_tex, 0, 0
			BrushTexture tx_pinsel(t), tx_bild(t), 0, 1
			BrushFX tx_pinsel(t), 2
		EndIf
	Next
	AmbientLight 255, 255, 255
End Function

; Nur die Eigenschaften, ohne Texturen - braucht keine Grafik.
; Haerte und Hand wie in Minecraft (Java Edition): mit der Hand dauert ein
; Block 1,5 x Haerte Sekunden, wenn die Hand genuegt, sonst 5 x Haerte -
; und dann bleibt nichts uebrig. Stein bricht nach 7,5 s zu nichts. Das
; passende Werkzeug teilt die Zeit (abbau.bb).
; Pflanzen haben Haerte 0 und brechen sofort; Luft, Wasser und Grundgestein
; nie (-1).
Function Bloecke_Arten()
	Block_Art(B_LUFT, "Luft", 0, 0, 0, False, False, -1, False)
	Block_Art(B_STEIN, "Stein", T_STEIN, T_STEIN, T_STEIN, True, True, 1.5, False)
	Block_Art(B_ERDE, "Erde", T_ERDE, T_ERDE, T_ERDE, True, True, 0.5, True)
	Block_Art(B_GRAS, "Gras", T_GRAS_OBEN, T_GRAS_SEITE, T_ERDE, True, True, 0.6, True)
	Block_Art(B_SAND, "Sand", T_SAND, T_SAND, T_SAND, True, True, 0.5, True)
	Block_Art(B_WASSER, "Wasser", T_WASSER, T_WASSER, T_WASSER, False, False, -1, False)
	Block_Art(B_KIES, "Kies", T_KIES, T_KIES, T_KIES, True, True, 0.6, True)
	Block_Art(B_STAMM, "Stamm", T_STAMM_OBEN, T_STAMM_SEITE, T_STAMM_OBEN, True, True, 2.0, True)
	Block_Art(B_LAUB, "Laub", T_LAUB, T_LAUB, T_LAUB, True, True, 0.2, True)
	Block_Art(B_GRUND, "Grundgestein", T_GRUND, T_GRUND, T_GRUND, True, True, -1, False)
	Block_Art(B_SCHNEE, "Schnee", T_SCHNEE, T_SCHNEE_SEITE, T_ERDE, True, True, 0.6, True)
	Block_Art(B_TON, "Ton", T_TON, T_TON, T_TON, True, True, 0.6, True)
	Block_Art(B_KOHLE, "Kohleerz", T_KOHLE, T_KOHLE, T_KOHLE, True, True, 3.0, False)
	Block_Art(B_EISEN, "Eisenerz", T_EISEN, T_EISEN, T_EISEN, True, True, 3.0, False)
	Block_Art(B_SANDSTEIN, "Sandstein", T_SANDSTEIN, T_SANDSTEIN, T_SANDSTEIN, True, True, 0.8, False)
	Block_Art(B_BRETTER, "Bretter", T_BRETTER, T_BRETTER, T_BRETTER, True, True, 2.0, True)
	Block_Art(B_WERKBANK, "Werkbank", T_WERKBANK_OBEN, T_WERKBANK_SEITE, T_BRETTER, True, True, 2.5, True)
	Block_Art(B_STEINZIEGEL, "Steinziegel", T_STEINZIEGEL, T_STEINZIEGEL, T_STEINZIEGEL, True, True, 1.5, False)
	Block_Art(B_OFEN, "Schmelzofen", T_OFEN_OBEN, T_OFEN_SEITE, T_STEIN, True, True, 3.5, False)
	Pflanzen_Art(B_HOHESGRAS, "Hohes Gras", T_HOHESGRAS)
	Pflanzen_Art(B_LOEWENZAHN, "Loewenzahn", T_LOEWENZAHN)
	Pflanzen_Art(B_MOHN, "Mohn", T_MOHN)
	Pflanzen_Art(B_KORNBLUME, "Kornblume", T_KORNBLUME)
	Pflanzen_Art(B_SCHILF, "Schilf", T_SCHILF)
	Pflanzen_Art(B_STEINE, "Steine", T_STEIN)   ; netz.bb baut kleine Klumpen
	Fackel_Art(B_FACKEL, "Fackel", 1, 0, 0)
	Fackel_Art(B_FACKEL_XP, "Fackel", 2, 1, 0)
	Fackel_Art(B_FACKEL_XM, "Fackel", 2, -1, 0)
	Fackel_Art(B_FACKEL_ZP, "Fackel", 2, 0, 1)
	Fackel_Art(B_FACKEL_ZM, "Fackel", 2, 0, -1)

	; Werkzeuge wie in Minecraft: die Spitzhacke fuer Gestein und Erze (erst
	; sie bringt dort Ertrag, schon die Steinspitzhacke auch bei Eisenerz),
	; die Axt fuer Holz, die Schaufel fuer lockeren Boden.
	For b = B_STEIN To B_ANZAHL - 1
		Select b
		Case B_STEIN, B_KOHLE, B_EISEN, B_SANDSTEIN, B_STEINZIEGEL, B_OFEN : Block_Werkzeug(b, W_HACKE, 1)
		Case B_STAMM, B_BRETTER, B_WERKBANK : Block_Werkzeug(b, W_AXT, 0)
		Case B_ERDE, B_GRAS, B_SAND, B_KIES, B_SCHNEE, B_TON : Block_Werkzeug(b, W_SCHAUFEL, 0)
		End Select
	Next
End Function

Function Block_Werkzeug(b, w, stufe)
	bl_werkzeug(b) = w
	bl_stufe(b) = stufe
End Function

; Kann Pflanze p auf Block unten stehen? Blumen und Gras wachsen auf Gras
; und Erde, Schilf auch auf Sand und auf Schilf, lose Steine auf allem
; Festen ausser Laub.
Function Pflanze_Haelt(p, unten)
	If p = B_STEINE Then Return bl_fest(unten) And unten <> B_LAUB
	If unten = B_GRAS Or unten = B_ERDE Then Return True
	If p = B_SCHILF And (unten = B_SAND Or unten = B_SCHILF) Then Return True
	Return False
End Function

; ---- Malen -------------------------------------------------------------

Function Rgb(r, g, b)
	If r < 0 Then r = 0
	If r > 255 Then r = 255
	If g < 0 Then g = 0
	If g > 255 Then g = 255
	If b < 0 Then b = 0
	If b > 255 Then b = 255
	Return $FF000000 Or (r Shl 16) Or (g Shl 8) Or b
End Function

; Grundfarbe mit zufaelliger Helligkeit je Texel.
Function Tx_Flaeche(r, g, b, streu)
	For y = 0 To 15
		For x = 0 To 15
			d = Rand(-streu, streu)
			tx_raster(x, y) = Rgb(r + d, g + d, b + d)
		Next
	Next
End Function

Function Tx_Punkt(x, y, r, g, b, streu)
	d = Rand(-streu, streu)
	tx_raster(x And 15, y And 15) = Rgb(r + d, g + d, b + d)
End Function

; Kleine Klumpen, etwa fuer Erze.
Function Tx_Klumpen(anzahl, r, g, b)
	For i = 1 To anzahl
		x = Rand(1, 13) : y = Rand(1, 13)
		Tx_Punkt(x, y, r, g, b, 15)
		Tx_Punkt(x + 1, y, r, g, b, 15)
		Tx_Punkt(x, y + 1, r, g, b, 15)
		If Rand(0, 1) Then Tx_Punkt(x + 1, y + 1, r, g, b, 15)
	Next
End Function

Function Tx_Gras_Rand(r, g, b)
	For x = 0 To 15
		tiefe = Rand(3, 5)
		For y = 0 To tiefe - 1
			Tx_Punkt(x, y, r, g, b, 14)
		Next
	Next
End Function

Function Tex_Malen(t)
	Select t
	Case T_STEIN
		Tx_Flaeche(124, 124, 124, 12)
		For i = 1 To 10
			Tx_Punkt(Rand(0, 15), Rand(0, 15), 100, 100, 100, 6)
		Next
	Case T_ERDE
		Tx_Flaeche(134, 96, 67, 14)
		For i = 1 To 12
			Tx_Punkt(Rand(0, 15), Rand(0, 15), 110, 76, 50, 8)
		Next
	Case T_GRAS_OBEN
		Tx_Flaeche(98, 160, 56, 18)
	Case T_GRAS_SEITE
		Tex_Malen(T_ERDE)
		Tx_Gras_Rand(98, 160, 56)
	Case T_SAND
		Tx_Flaeche(220, 208, 162, 9)
	Case T_WASSER
		Tx_Flaeche(48, 92, 205, 8)
		For i = 1 To 8
			x = Rand(0, 15) : y = Rand(0, 15)
			For k = 0 To Rand(2, 4)
				Tx_Punkt(x + k, y, 80, 130, 230, 6)
			Next
		Next
	Case T_KIES
		For y = 0 To 15 Step 2
			For x = 0 To 15 Step 2
				g = Rand(95, 160)
				For dy = 0 To 1
					For dx = 0 To 1
						Tx_Punkt(x + dx, y + dy, g, g - 4, g - 8, 8)
					Next
				Next
			Next
		Next
	Case T_STAMM_SEITE
		For x = 0 To 15
			s = Rand(-14, 14)
			If (x Mod 4) = 0 Then s = s - 22
			For y = 0 To 15
				Tx_Punkt(x, y, 104 + s, 82 + s, 50 + s, 5)
			Next
		Next
	Case T_STAMM_OBEN
		For y = 0 To 15
			For x = 0 To 15
				d# = Sqr((x - 7.5) * (x - 7.5) + (y - 7.5) * (y - 7.5))
				If d > 6.8
					Tx_Punkt(x, y, 104, 82, 50, 8)
				ElseIf (Int(d) Mod 2) = 0
					Tx_Punkt(x, y, 182, 146, 94, 6)
				Else
					Tx_Punkt(x, y, 160, 126, 78, 6)
				EndIf
			Next
		Next
	Case T_LAUB
		Tx_Flaeche(58, 128, 40, 20)
		For i = 1 To 40
			Tx_Punkt(Rand(0, 15), Rand(0, 15), 34, 84, 24, 8)
		Next
	Case T_GRUND
		For y = 0 To 15
			For x = 0 To 15
				If Rand(0, 2) = 0 Then g = 40 Else g = Rand(80, 120)
				Tx_Punkt(x, y, g, g, g, 6)
			Next
		Next
	Case T_SCHNEE
		Tx_Flaeche(242, 248, 252, 5)
	Case T_SCHNEE_SEITE
		Tex_Malen(T_ERDE)
		Tx_Gras_Rand(242, 248, 252)
	Case T_TON
		Tx_Flaeche(160, 166, 180, 7)
	Case T_KOHLE
		Tex_Malen(T_STEIN)
		Tx_Klumpen(5, 30, 30, 30)
	Case T_EISEN
		Tex_Malen(T_STEIN)
		Tx_Klumpen(5, 216, 172, 140)
	Case T_SANDSTEIN
		For y = 0 To 15
			s = 0
			If (y Mod 5) = 4 Then s = -18
			For x = 0 To 15
				Tx_Punkt(x, y, 216 + s, 200 + s, 150 + s, 6)
			Next
		Next
	Case T_BRETTER
		; vier waagerechte Bretter mit dunklen Fugen, die Stoesse versetzt
		For y = 0 To 15
			reihe = y / 4
			If (y Mod 4) = 0 Then s = Rand(-10, 10)
			For x = 0 To 15
				If (y Mod 4) = 3
					Tx_Punkt(x, y, 118, 86, 50, 4)
				ElseIf x = ((reihe * 7 + 3) Mod 16)
					Tx_Punkt(x, y, 128, 94, 56, 4)
				Else
					Tx_Punkt(x, y, 182 + s, 142 + s, 90 + s, 6)
				EndIf
			Next
		Next
	Case T_WERKBANK_OBEN
		; Bretter mit dunklem Rahmen und einem Raster aus Fugen darauf
		Tex_Malen(T_BRETTER)
		For k = 0 To 15
			Tx_Punkt(k, 0, 92, 64, 36, 4) : Tx_Punkt(k, 15, 92, 64, 36, 4)
			Tx_Punkt(0, k, 92, 64, 36, 4) : Tx_Punkt(15, k, 92, 64, 36, 4)
			If k > 1 And k < 14
				Tx_Punkt(k, 5, 120, 86, 50, 4) : Tx_Punkt(k, 10, 120, 86, 50, 4)
				Tx_Punkt(5, k, 120, 86, 50, 4) : Tx_Punkt(10, k, 120, 86, 50, 4)
			EndIf
		Next
	Case T_WERKBANK_SEITE
		; Bretter, oben die dunkle Kante der Platte, davor Hammer und Saege
		Tex_Malen(T_BRETTER)
		For x = 0 To 15
			Tx_Punkt(x, 0, 120, 86, 50, 4) : Tx_Punkt(x, 1, 150, 110, 64, 4) : Tx_Punkt(x, 2, 92, 64, 36, 4)
		Next
		For y = 6 To 13
			Tx_Punkt(4, y, 104, 70, 38, 4)
		Next
		For x = 2 To 6
			Tx_Punkt(x, 5, 150, 150, 150, 6) : Tx_Punkt(x, 6, 110, 110, 110, 6)
		Next
		For y = 7 To 11
			For x = 9 To 13
				Tx_Punkt(x, y, 186, 186, 190, 6)
			Next
		Next
		For x = 9 To 13 Step 2
			Tx_Punkt(x, 12, 150, 150, 156, 4)
		Next
		Tx_Punkt(14, 7, 104, 70, 38, 4) : Tx_Punkt(14, 8, 104, 70, 38, 4) : Tx_Punkt(14, 9, 104, 70, 38, 4)
	Case T_STEINZIEGEL
		; Ziegel in Reihen zu vier Texeln, die Stoesse versetzt, helle Fugen
		For y = 0 To 15
			reihe = y / 4
			For x = 0 To 15
				If (y Mod 4) = 3 Or ((x + (reihe Mod 2) * 4) Mod 8) = 7
					Tx_Punkt(x, y, 156, 156, 152, 5)
				Else
					Tx_Punkt(x, y, 118, 118, 120, 9)
				EndIf
			Next
		Next
	Case T_OFEN_SEITE
		; Steinziegel mit einer Feueroeffnung, unten darin Glut
		Tex_Malen(T_STEINZIEGEL)
		For y = 7 To 14
			For x = 3 To 12
				If y = 7 Or x = 3 Or x = 12 Or y = 14
					Tx_Punkt(x, y, 70, 70, 72, 4)
				ElseIf y >= 12
					If Rand(0, 2) = 0 Then Tx_Punkt(x, y, 255, 214, 90, 10) Else Tx_Punkt(x, y, 236, 110, 30, 14)
				ElseIf y = 11
					If Rand(0, 1) = 0 Then Tx_Punkt(x, y, 200, 70, 20, 10) Else Tx_Punkt(x, y, 24, 20, 20, 4)
				Else
					Tx_Punkt(x, y, 24, 20, 20, 4)
				EndIf
			Next
		Next
	Case T_OFEN_OBEN
		; Steinziegel, in der Mitte der Abzug
		Tex_Malen(T_STEINZIEGEL)
		For y = 5 To 10
			For x = 5 To 10
				If y = 5 Or x = 5 Or y = 10 Or x = 10 Then Tx_Punkt(x, y, 70, 70, 72, 4) Else Tx_Punkt(x, y, 30, 26, 26, 4)
			Next
		Next
	Case T_HOHESGRAS
		; Halme verschiedener Hoehe, von unten nach oben heller, leicht schief
		Tx_Leer()
		For i = 1 To 11
			x = Rand(0, 15) : hoch = Rand(6, 14) : schief = Rand(-1, 1)
			For k = 0 To hoch - 1
				hell = k * 40 / hoch
				Tx_Punkt(x + (k * schief) / 5, 15 - k, 60 + hell, 120 + hell, 36 + hell / 2, 10)
			Next
		Next
	Case T_LOEWENZAHN
		Tx_Blume(232, 204, 36, 2, 255, 236, 110)
	Case T_MOHN
		Tx_Blume(196, 34, 30, 3, 40, 24, 20)
	Case T_KORNBLUME
		Tx_Blume(70, 100, 220, 3, 230, 230, 240)
	Case T_FACKEL
		; ein Stab aus Holz, oben die Flamme
		Tx_Leer()
		For y = 6 To 15
			Tx_Punkt(7, y, 122, 88, 52, 6) : Tx_Punkt(8, y, 100, 70, 40, 6)
		Next
		Tx_Punkt(7, 5, 255, 246, 190, 0) : Tx_Punkt(8, 5, 255, 226, 130, 0)
		Tx_Punkt(7, 4, 255, 210, 90, 0) : Tx_Punkt(8, 4, 255, 236, 150, 0)
		Tx_Punkt(7, 3, 255, 160, 40, 0) : Tx_Punkt(8, 3, 250, 130, 30, 0)
		Tx_Punkt(6, 5, 236, 120, 30, 0) : Tx_Punkt(9, 5, 236, 120, 30, 0)
		Tx_Punkt(7, 2, 230, 100, 20, 0)
	Case T_SCHILF
		; drei Rohre mit dunkleren Knoten, oben und unten offen - uebereinander
		; gestapelt laufen sie durch
		Tx_Leer()
		For r = 0 To 2
			x = 2 + r * 5 + Rand(0, 1)
			knoten = Rand(0, 3)
			For y = 0 To 15
				If ((y + knoten) Mod 5) = 0
					Tx_Punkt(x, y, 104, 150, 62, 6) : Tx_Punkt(x + 1, y, 92, 138, 54, 6)
				Else
					Tx_Punkt(x, y, 150, 204, 102, 8) : Tx_Punkt(x + 1, y, 128, 184, 86, 8)
				EndIf
			Next
			; ein Blatt
			y = Rand(3, 11)
			Tx_Punkt(x + 2, y, 120, 176, 80, 8) : Tx_Punkt(x + 3, y - 1, 120, 176, 80, 8)
		Next
	Default
		Ding_Tex_Malen(t)
	End Select
End Function

; Die Lichtrampe: links Schwarz, rechts das warme Licht einer Fackel. Die
; zweite Texturkoordinate eines Vertex waehlt die Stelle (netz.bb).
Function Licht_Rampe()
	tex = CreateTexture(256, 4, 1 + 16 + 32)
	TextureBlend tex, 3
	TextureCoords tex, 1
	puffer = TextureBuffer(tex)
	LockBuffer puffer
	For x = 0 To 255
		For y = 0 To 3
			WritePixelFast x, y, Rgb(x, x * 0.86, x * 0.64), puffer
		Next
	Next
	UnlockBuffer puffer
	Return tex
End Function

; Weisse Kopie einer maskierten Textur: Alpha bleibt, Farbe wird Weiss.
Function Tex_Maske(quelle)
	g = TextureWidth(quelle)
	tex = CreateTexture(g, g, 1 + 4 + 8)
	LockBuffer TextureBuffer(quelle)
	LockBuffer TextureBuffer(tex)
	For y = 0 To g - 1
		For x = 0 To g - 1
			c = ReadPixelFast(x, y, TextureBuffer(quelle))
			WritePixelFast x, y, (c And $FF000000) Or $FFFFFF, TextureBuffer(tex)
		Next
	Next
	UnlockBuffer TextureBuffer(tex)
	UnlockBuffer TextureBuffer(quelle)
	Return tex
End Function

; Alles durchsichtig (Alpha 0) - Grund fuer Pflanzen.
Function Tx_Leer()
	For y = 0 To 15
		For x = 0 To 15
			tx_raster(x, y) = 0
		Next
	Next
End Function

; Blume: Stiel mit zwei Blaettern, oben die Bluete in Farbe r, g, b mit
; einer Mitte in Farbe mr, mg, mb; gross ist der Radius der Bluete.
Function Tx_Blume(r, g, b, gross, mr, mg, mb)
	Tx_Leer()
	x = 7 + Rand(0, 1)
	For y = 7 To 15
		Tx_Punkt(x, y, 70, 132, 40, 8)
	Next
	Tx_Punkt(x - 1, 12, 78, 146, 46, 8) : Tx_Punkt(x - 2, 11, 78, 146, 46, 8)
	Tx_Punkt(x + 1, 10, 78, 146, 46, 8) : Tx_Punkt(x + 2, 9, 78, 146, 46, 8)
	my = 7 - gross / 2
	For dy = -gross To gross
		For dx = -gross To gross
			If dx * dx + dy * dy <= gross * gross + 1
				Tx_Punkt(x + dx, my + dy, r, g, b, 14)
			EndIf
		Next
	Next
	Tx_Punkt(x, my, mr, mg, mb, 6)
End Function

Function Tex_Aus_Raster(maske = False)
	g = 16 * TEX_ZOOM
	If maske Then tex = CreateTexture(g, g, 1 + 4 + 8) Else tex = CreateTexture(g, g, 1 + 8)
	puffer = TextureBuffer(tex)
	LockBuffer puffer
	For y = 0 To g - 1
		For x = 0 To g - 1
			WritePixelFast x, y, tx_raster(x / TEX_ZOOM, y / TEX_ZOOM), puffer
		Next
	Next
	UnlockBuffer puffer
	Return tex
End Function

; ---- Texturen der Gegenstaende (Pixelkunst 16 x 16, durchsichtig) --------

Function Tx_Linie(x0, y0, x1, y1, r, g, b, streu)
	n = Abs(x1 - x0) : If Abs(y1 - y0) > n Then n = Abs(y1 - y0)
	For k = 0 To n
		If n = 0 Then f# = 0 Else f# = Float(k) / n
		Tx_Punkt(Floor(x0 + (x1 - x0) * f + 0.5), Floor(y0 + (y1 - y0) * f + 0.5), r, g, b, streu)   ; Int rundet in Blitz
	Next
End Function

; Ein runder Steinklumpen, oben links heller, unten rechts dunkler.
Function Tx_Kiesel(cx#, cy#, rad#, grau)
	For y = 0 To 15
		For x = 0 To 15
			dx# = x + 0.5 - cx : dy# = y + 0.5 - cy
			If dx * dx + dy * dy <= rad * rad
				licht = Int(-(dx + dy) * 9)
				If dx * dx + dy * dy > (rad - 1) * (rad - 1) And dx + dy > 0 Then licht = licht - 22
				Tx_Punkt(x, y, grau + licht, grau + licht, grau + licht - 4, 7)
			EndIf
		Next
	Next
End Function

; Stiel von links unten nach rechts oben: drei Bildpunkte breit, oben hell,
; unten dunkel fuer etwas Rundung.
Function Tx_Stiel(x0, y0, x1, y1)
	Tx_Linie(x0, y0 - 1, x1, y1 - 1, 176, 128, 74, 6)
	Tx_Linie(x0, y0, x1, y1, 148, 104, 58, 6)
	Tx_Linie(x0 + 1, y0, x1 + 1, y1, 104, 70, 38, 5)
End Function

Dim tx_kopfmaske(15, 15)

; Werkzeugkopf aus Stein um (cx, cy). Koordinaten im gedrehten System:
; a entlang des Stiels (nach rechts oben), q quer dazu (nach rechts unten).
; art: W_AXT, W_HACKE oder W_SCHAUFEL. Der Rand wird dunkel, damit der
; graue Stein auch vor grauem Grund lesbar bleibt.
Function Tx_Kopf(art, cx#, cy#, eisen = False)
	For y = 0 To 15
		For x = 0 To 15
			dx# = x + 0.5 - cx : dy# = y + 0.5 - cy
			a# = (dx - dy) * 0.7071 : q# = (dx + dy) * 0.7071
			drin = False
			Select art
			Case W_AXT
				; schmal am Stiel, zur Schneide (quer, nach rechts unten) breiter
				If q >= -2.0 And q <= 4.6 And Abs(a) <= 1.6 + Tx_Klemmen(q, 0, 5) * 0.5 Then drin = True
			Case W_HACKE
				; ein langer Bogen quer zum Stiel, die Enden etwas zurueck
				If Abs(q) <= 6.2 And a <= 1.3 - q * q * 0.045 And a >= -1.3 - q * q * 0.045 Then drin = True
			Case W_SCHAUFEL
				; ein Blatt in Verlaengerung des Stiels, vorn abgerundet
				If a >= -1.5 And a <= 4.8 And Abs(q) <= 2.6 - Tx_Klemmen(a - 3.0, 0, 2) * 0.9 Then drin = True
			End Select
			tx_kopfmaske(x, y) = drin
		Next
	Next
	For y = 0 To 15
		For x = 0 To 15
			If tx_kopfmaske(x, y)
				rand = False
				If x = 0 Or x = 15 Or y = 0 Or y = 15
					rand = True
				ElseIf Not (tx_kopfmaske(x - 1, y) And tx_kopfmaske(x + 1, y) And tx_kopfmaske(x, y - 1) And tx_kopfmaske(x, y + 1))
					rand = True
				EndIf
				If rand
					If eisen Then Tx_Punkt(x, y, 92, 94, 104, 5) Else Tx_Punkt(x, y, 70, 70, 68, 6)
				Else
					dx# = x + 0.5 - cx : dy# = y + 0.5 - cy
					licht = Int(-(dx + dy) * 6)
					If eisen Then Tx_Punkt(x, y, 206 + licht, 208 + licht, 214 + licht, 6) Else Tx_Punkt(x, y, 156 + licht, 156 + licht, 150 + licht, 10)
				EndIf
			EndIf
		Next
	Next
End Function

; Wird von Tex_Malen fuer alle Texturen ab T_AST gerufen.
Function Ding_Tex_Malen(t)
	Tx_Leer()
	Select t
	Case T_AST
		Tx_Stiel(3, 14, 12, 3)
		Tx_Linie(8, 8, 12, 9, 148, 104, 58, 6)
		Tx_Punkt(13, 9, 84, 160, 50, 10) : Tx_Punkt(13, 10, 70, 140, 40, 10) : Tx_Punkt(12, 10, 84, 160, 50, 10)
		Tx_Punkt(13, 2, 84, 160, 50, 10) : Tx_Punkt(14, 2, 70, 140, 40, 10)
	Case T_BROCKEN
		Tx_Kiesel(5.5, 10.5, 3.2, 136)
		Tx_Kiesel(11.0, 11.5, 2.8, 120)
		Tx_Kiesel(8.5, 5.5, 2.6, 150)
	Case T_STEINAXT
		Tx_Stiel(1, 15, 11, 5)
		Tx_Kopf(W_AXT, 10.0, 5.0)
	Case T_STEINHACKE
		Tx_Stiel(1, 15, 11, 5)
		Tx_Kopf(W_HACKE, 10.5, 5.5)
	Case T_STEINSCHAUFEL
		Tx_Stiel(1, 15, 8, 8)
		Tx_Kopf(W_SCHAUFEL, 10.0, 6.0)
	Case T_KOHLESTUECK
		Tx_Kiesel(6.0, 10.0, 3.4, 52)
		Tx_Kiesel(10.8, 8.0, 3.0, 44)
		Tx_Punkt(5, 8, 120, 120, 128, 0) : Tx_Punkt(10, 6, 110, 110, 120, 0)
	Case T_EISENBARREN
		; ein Barren schraeg von oben: helle Oberseite, dunklere Vorderseite
		For y = 5 To 11
			For x = 1 To 14
				If y <= 7
					; Oberseite als Parallelogramm, nach rechts versetzt
					If x >= 4 + (7 - y) And x <= 11 + (7 - y) Then Tx_Punkt(x, y, 226, 228, 234, 5)
				ElseIf x >= 3 And x <= 12
					If y = 11 Or x = 3 Or x = 12 Then Tx_Punkt(x, y, 110, 112, 120, 4) Else Tx_Punkt(x, y, 178, 180, 188, 6)
				EndIf
			Next
		Next
		For x = 13 To 14 : Tx_Punkt(x, 9 - (x - 13), 140, 142, 150, 4) : Tx_Punkt(x, 10 - (x - 13), 140, 142, 150, 4) : Next
	Case T_EISENAXT
		Tx_Stiel(1, 15, 11, 5)
		Tx_Kopf(W_AXT, 10.0, 5.0, True)
	Case T_EISENHACKE
		Tx_Stiel(1, 15, 11, 5)
		Tx_Kopf(W_HACKE, 10.5, 5.5, True)
	Case T_EISENSCHAUFEL
		Tx_Stiel(1, 15, 8, 8)
		Tx_Kopf(W_SCHAUFEL, 10.0, 6.0, True)
	End Select
End Function

Function Tx_Klemmen#(x#, a#, b#)
	If x < a Then Return a
	If x > b Then Return b
	Return x
End Function
