; BLTZCRFT - Netze
;
; Aus den Bloecken eines Chunks werden zwei Meshes: eines fuer die festen
; Bloecke, eines fuer das Wasser (durchscheinend, beidseitig). Gezeichnet wird
; nur eine Flaeche, hinter der kein deckender Block liegt. Jede Textur hat
; ihre eigene Surface.
;
; Helligkeit steckt in den Vertexfarben, Lichtquellen gibt es keine:
;   - je Richtung eine feste Helligkeit (oben hell, unten dunkel),
;   - Umgebungsschatten an jeder Ecke aus den drei Nachbarzellen davor,
;   - Himmelslicht: liegt ueber der Zelle vor der Flaeche ein deckender
;     Block, wird es dunkler, je tiefer sie darunter liegt - Hoehlen und
;     der Boden unter Baeumen. Gemittelt ueber die vier Zellen an der Ecke,
;     damit Schattenkanten weich auslaufen.
; Ein Chunk wird erst gebaut, wenn alle acht Nachbarn geladen sind - an den
; Raendern braucht er deren Bloecke.
;
; Zum Bauen kommt der Chunk samt einem Block Rand in ein eindimensionales
; Feld: Index = X + Z * 18 + Y * 324 mit X = x + 1, Z = z + 1, Y = y + 1.
; Ein Nachbar ist dann nur ein fester Versatz. (Mehrdimensionale Felder sind
; in BLTZNXT deutlich langsamer, siehe PROTOKOLL.md.)

Const NZ_Z = 18
Const NZ_Y = 324

Dim fl_n(17)                ; Richtung je Flaeche: x, y, z
Dim fl_ni(5)                ; dieselbe Richtung als Versatz im Feld
Dim fl_ex(23)               ; Ecken (oben links, oben rechts, unten rechts,
Dim fl_ey(23)               ; unten links) je Flaeche * 4 + Ecke
Dim fl_ez(23)
Dim fl_ao(71)               ; je Flaeche * 12 + Ecke * 3: Seite 1, Seite 2,
                            ; Ecke - als Versatz im Feld
Dim fl_licht#(5)
Dim fl_u#(3)
Dim fl_v#(3)
Dim ao_stufe#(3)

Dim nb_bank(2, 2)           ; Banks des Chunks und seiner Nachbarn
Dim nz_b(NZ_Y * 130)        ; Bloecke des Chunks samt Rand
Dim nz_d(NZ_Y * 130)        ; dasselbe: deckend ja/nein
Dim nz_hm(NZ_Y - 1)         ; oberster deckender Block je Saeule (-1 keiner)
Dim nz_fl(T_ANZAHL - 1)     ; Surface je Textur im entstehenden Mesh

Dim ecke_ao(3)
Dim ecke_hell#(3)

Global wasser_pinsel
Global nz_aktuell           ; Mesh, an dem gerade gebaut wird
Global nz_bloecke = 0       ; Statistik: gebaute Flaechen beim letzten Chunk

Function Netz_Laden()
	Restore flaechen_daten
	For f = 0 To 5
		Read fl_n(f * 3), fl_n(f * 3 + 1), fl_n(f * 3 + 2), fl_licht(f)
		fl_ni(f) = fl_n(f * 3) + fl_n(f * 3 + 2) * NZ_Z + fl_n(f * 3 + 1) * NZ_Y
		For k = 0 To 3
			Read fl_ex(f * 4 + k), fl_ey(f * 4 + k), fl_ez(f * 4 + k)
		Next
	Next
	fl_u(0) = 0 : fl_v(0) = 0
	fl_u(1) = 1 : fl_v(1) = 0
	fl_u(2) = 1 : fl_v(2) = 1
	fl_u(3) = 0 : fl_v(3) = 1
	ao_stufe(0) = 0.5 : ao_stufe(1) = 0.66 : ao_stufe(2) = 0.83 : ao_stufe(3) = 1.0

	; Versaetze fuer den Umgebungsschatten: die Zelle vor der Flaeche, dann
	; je Achse quer zur Flaeche einen Schritt zur Ecke hin.
	For f = 0 To 5
		For k = 0 To 3
			s1 = fl_ni(f) : s2 = fl_ni(f) : erste = True
			For a = 0 To 2
				If fl_n(f * 3 + a) = 0
					Select a
					Case 0 : s = (fl_ex(f * 4 + k) * 2 - 1)
					Case 1 : s = (fl_ey(f * 4 + k) * 2 - 1) * NZ_Y
					Case 2 : s = (fl_ez(f * 4 + k) * 2 - 1) * NZ_Z
					End Select
					If erste
						s1 = s1 + s : erste = False
					Else
						s2 = s2 + s
					EndIf
				EndIf
			Next
			fl_ao(f * 12 + k * 3) = s1
			fl_ao(f * 12 + k * 3 + 1) = s2
			fl_ao(f * 12 + k * 3 + 2) = s1 + s2 - fl_ni(f)
		Next
	Next

	wasser_pinsel = CreateBrush()
	BrushTexture wasser_pinsel, tx_bild(T_WASSER)
	BrushFX wasser_pinsel, 1 + 2 + 16
	BrushAlpha wasser_pinsel, 0.72
End Function

.flaechen_daten
; Richtung x, y, z, Helligkeit; dann vier Ecken im Uhrzeigersinn von aussen
Data 0, 1, 0, 1.0,   0, 1, 1,  1, 1, 1,  1, 1, 0,  0, 1, 0
Data 0, -1, 0, 0.5,  0, 0, 0,  1, 0, 0,  1, 0, 1,  0, 0, 1
Data 0, 0, 1, 0.8,   1, 1, 1,  0, 1, 1,  0, 0, 1,  1, 0, 1
Data 0, 0, -1, 0.8,  0, 1, 0,  1, 1, 0,  1, 0, 0,  0, 0, 0
Data 1, 0, 0, 0.65,  1, 1, 0,  1, 1, 1,  1, 0, 1,  1, 0, 0
Data -1, 0, 0, 0.65, 0, 1, 1,  0, 1, 0,  0, 0, 0,  0, 0, 1

; Himmelslicht der Zelle mit diesem Feldindex
Function Himmel#(i)
	d = nz_hm(i Mod NZ_Y) - (i / NZ_Y - 1)
	If d < 0 Then Return 1.0
	h# = 0.8 - d * 0.05
	If h < 0.34 Then h = 0.34
	Return h
End Function

Function Chunk_Baubar(c.Chunk)
	For dz = -1 To 1
		For dx = -1 To 1
			If Chunk_Holen(c\cx + dx, c\cz + dz) = Null Then Return False
		Next
	Next
	Return True
End Function

Function Chunk_Bauen(c.Chunk)
	For dz = -1 To 1
		For dx = -1 To 1
			n.Chunk = Chunk_Holen(c\cx + dx, c\cz + dz)
			nb_bank(dx + 1, dz + 1) = n\daten
			If n\hmax > hmax Then hmax = n\hmax
		Next
	Next
	If hmax > HOEHE - 2 Then hmax = HOEHE - 2

	; Den Chunk samt Rand einmal ins Feld holen, dabei das Himmelslicht:
	; oberster deckender Block je Saeule.
	For z = -1 To 16
		For x = -1 To 16
			bank = nb_bank((x + 16) Shr 4, (z + 16) Shr 4)
			o = ((z And 15) Shl 4) Or (x And 15)
			spalte = (x + 1) + (z + 1) * NZ_Z
			hm = -1
			i = spalte + (hmax + 2) * NZ_Y
			For y = hmax + 1 To 0 Step -1
				b = PeekByte(bank, (y Shl 8) Or o)
				nz_b(i) = b
				nz_d(i) = bl_deckend(b)
				If hm < 0 And nz_d(i) Then hm = y
				i = i - NZ_Y
			Next
			nz_b(spalte) = B_GRUND : nz_d(spalte) = True
			i = spalte + (hmax + 3) * NZ_Y
			nz_b(i) = B_LUFT : nz_d(i) = False
			nz_hm(spalte) = hm
		Next
	Next

	If c\netz Then FreeEntity c\netz
	If c\wasser Then FreeEntity c\wasser
	c\netz = CreateMesh()
	nz_aktuell = c\netz
	c\wasser = 0
	ws = 0
	For t = 0 To T_ANZAHL - 1 : nz_fl(t) = 0 : Next
	nz_bloecke = 0

	For y = 0 To c\hmax
		For z = 0 To 15
			i = 1 + (z + 1) * NZ_Z + (y + 1) * NZ_Y
			For x = 0 To 15
				b = nz_b(i)
				If b = B_WASSER
					For f = 0 To 5
						nb = nz_b(i + fl_ni(f))
						If nb <> B_WASSER And (Not bl_deckend(nb))
							If f <> 1 Or nb = B_LUFT
								If ws = 0
									c\wasser = CreateMesh()
									ws = CreateSurface(c\wasser, wasser_pinsel)
								EndIf
								Wasser_Flaeche(ws, x, y, z, i, f)
							EndIf
						EndIf
					Next
				ElseIf b <> B_LUFT
					If Not nz_d(i + NZ_Y) Then Block_Flaeche(b, x, y, z, i, 0)
					If Not nz_d(i - NZ_Y) Then Block_Flaeche(b, x, y, z, i, 1)
					If Not nz_d(i + NZ_Z) Then Block_Flaeche(b, x, y, z, i, 2)
					If Not nz_d(i - NZ_Z) Then Block_Flaeche(b, x, y, z, i, 3)
					If Not nz_d(i + 1) Then Block_Flaeche(b, x, y, z, i, 4)
					If Not nz_d(i - 1) Then Block_Flaeche(b, x, y, z, i, 5)
				EndIf
				i = i + 1
			Next
		Next
	Next

	PositionEntity c\netz, c\cx * CH, 0, c\cz * CH
	If c\wasser Then PositionEntity c\wasser, c\cx * CH, 0, c\cz * CH
	c\veraltet = False
End Function

Function Block_Flaeche(b, x, y, z, i, f)
	Select f
	Case 0 : t = bl_tex(b, 0)
	Case 1 : t = bl_tex(b, 2)
	Default : t = bl_tex(b, 1)
	End Select
	s = nz_fl(t)
	If s = 0
		s = CreateSurface(nz_aktuell, tx_pinsel(t))
		nz_fl(t) = s
	EndIf
	nz_bloecke = nz_bloecke + 1

	vorn# = Himmel(i + fl_ni(f))
	licht# = fl_licht(f) * 255
	For k = 0 To 3
		a = f * 12 + k * 3
		i1 = i + fl_ao(a) : i2 = i + fl_ao(a + 1) : i3 = i + fl_ao(a + 2)
		s1 = nz_d(i1) : s2 = nz_d(i2) : ec = nz_d(i3)
		If s1 And s2
			ao = 0
		Else
			ao = 3 - s1 - s2 - ec
		EndIf
		ecke_ao(k) = ao

		; Himmelslicht der freien Zellen an dieser Ecke gemittelt
		summe# = vorn : n = 1
		If Not s1 Then summe = summe + Himmel(i1) : n = n + 1
		If Not s2 Then summe = summe + Himmel(i2) : n = n + 1
		If (Not ec) And (Not (s1 And s2)) Then summe = summe + Himmel(i3) : n = n + 1
		ecke_hell(k) = licht * ao_stufe(ao) * summe / n
	Next

	e = f * 4
	v0 = AddVertex(s, x + fl_ex(e), y + fl_ey(e), z + fl_ez(e), 0, 0)
	v1 = AddVertex(s, x + fl_ex(e + 1), y + fl_ey(e + 1), z + fl_ez(e + 1), 1, 0)
	v2 = AddVertex(s, x + fl_ex(e + 2), y + fl_ey(e + 2), z + fl_ez(e + 2), 1, 1)
	v3 = AddVertex(s, x + fl_ex(e + 3), y + fl_ey(e + 3), z + fl_ez(e + 3), 0, 1)
	VertexColor s, v0, ecke_hell(0), ecke_hell(0), ecke_hell(0)
	VertexColor s, v1, ecke_hell(1), ecke_hell(1), ecke_hell(1)
	VertexColor s, v2, ecke_hell(2), ecke_hell(2), ecke_hell(2)
	VertexColor s, v3, ecke_hell(3), ecke_hell(3), ecke_hell(3)
	; Die Diagonale so legen, dass der Schatten nicht schief verlaeuft.
	If ecke_ao(0) + ecke_ao(2) < ecke_ao(1) + ecke_ao(3)
		AddTriangle s, v0, v1, v3
		AddTriangle s, v1, v2, v3
	Else
		AddTriangle s, v0, v1, v2
		AddTriangle s, v0, v2, v3
	EndIf
End Function

Function Wasser_Flaeche(s, x, y, z, i, f)
	; Liegt kein Wasser darueber, steht der Spiegel etwas unter der Kante.
	oben# = 1.0
	If nz_b(i + NZ_Y) <> B_WASSER Then oben = 0.875
	hell# = fl_licht(f) * Himmel(i + fl_ni(f)) * 255
	For k = 0 To 3
		e = f * 4 + k
		ey# = fl_ey(e)
		If ey > 0.5 Then ey = oben
		ecke_ao(k) = AddVertex(s, x + fl_ex(e), y + ey, z + fl_ez(e), fl_u(k), fl_v(k))
		VertexColor s, ecke_ao(k), hell, hell, hell
	Next
	AddTriangle s, ecke_ao(0), ecke_ao(1), ecke_ao(2)
	AddTriangle s, ecke_ao(0), ecke_ao(2), ecke_ao(3)
End Function
