; BLTZCRFT - Bloecke
;
; Jede Blockart hat eine Nummer (ein Byte in der Welt), drei Texturen (oben,
; Seite, unten) und zwei Eigenschaften: deckend (verdeckt die Flaechen der
; Nachbarn und wirft Umgebungsschatten) und fest (man stoesst sich daran).
; Dazu wie in Minecraft die Haerte und ob die blosse Hand genuegt (abbau.bb).
; Pflanzen (Gras, Blumen, Schilf) sind weder deckend noch fest; sie stehen
; als zwei gekreuzte Flaechen in ihrer Zelle (netz.bb), ihre Texturen haben
; durchsichtige Stellen (Maske).
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
Const B_ANZAHL = 20

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
Const T_HOHESGRAS = 17
Const T_LOEWENZAHN = 18
Const T_MOHN = 19
Const T_KORNBLUME = 20
Const T_SCHILF = 21
Const T_ANZAHL = 22
Const T_ERSTE_PFLANZE = 17      ; ab hier Pflanzentexturen (mit Maske)

Const TEX_ZOOM = 8          ; Bildpunkte je Texel

Dim bl_tex(B_ANZAHL - 1, 2)     ; 0 oben, 1 Seite, 2 unten
Dim bl_deckend(B_ANZAHL - 1)
Dim bl_fest(B_ANZAHL - 1)
Dim bl_name$(B_ANZAHL - 1)
Dim bl_haerte#(B_ANZAHL - 1)    ; Haerte wie in Minecraft, 0 = sofort, -1 = nie
Dim bl_hand(B_ANZAHL - 1)       ; mit blosser Hand abbaubar (sonst kein Ertrag)
Dim bl_pflanze(B_ANZAHL - 1)    ; Kreuz aus zwei Flaechen statt Wuerfel

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
	bl_pflanze(nr) = False
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
	For t = 0 To T_ANZAHL - 1
		Tex_Malen(t)
		pflanze = (t >= T_ERSTE_PFLANZE)
		tx_bild(t) = Tex_Aus_Raster(pflanze)
		tx_pinsel(t) = CreateBrush()
		BrushTexture tx_pinsel(t), tx_bild(t)
		; Vertexfarben ohne volle Helligkeit: es gibt keine Lichtquelle, also
		; ist die Farbe Vertexfarbe mal Umgebungslicht - damit macht
		; himmel.bb die ganze Welt mit einem Befehl hell oder dunkel.
		; Pflanzen sieht man von beiden Seiten.
		If pflanze Then BrushFX tx_pinsel(t), 2 + 16 Else BrushFX tx_pinsel(t), 2
	Next
	AmbientLight 255, 255, 255
End Function

; Nur die Eigenschaften, ohne Texturen - braucht keine Grafik.
; Haerte und Hand wie in Minecraft (Java Edition): mit der Hand dauert ein
; Block 1,5 x Haerte Sekunden, wenn die Hand genuegt, sonst 5 x Haerte -
; und dann bleibt nichts uebrig. Stein bricht nach 7,5 s zu nichts.
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
	Block_Art(B_KOHLE, "Kohle", T_KOHLE, T_KOHLE, T_KOHLE, True, True, 3.0, False)
	Block_Art(B_EISEN, "Eisen", T_EISEN, T_EISEN, T_EISEN, True, True, 3.0, False)
	Block_Art(B_SANDSTEIN, "Sandstein", T_SANDSTEIN, T_SANDSTEIN, T_SANDSTEIN, True, True, 0.8, False)
	Pflanzen_Art(B_HOHESGRAS, "Hohes Gras", T_HOHESGRAS)
	Pflanzen_Art(B_LOEWENZAHN, "Loewenzahn", T_LOEWENZAHN)
	Pflanzen_Art(B_MOHN, "Mohn", T_MOHN)
	Pflanzen_Art(B_KORNBLUME, "Kornblume", T_KORNBLUME)
	Pflanzen_Art(B_SCHILF, "Schilf", T_SCHILF)
End Function

; Kann Pflanze p auf Block unten stehen? Blumen und Gras wachsen auf Gras
; und Erde, Schilf auch auf Sand und auf Schilf.
Function Pflanze_Haelt(p, unten)
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
	End Select
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
