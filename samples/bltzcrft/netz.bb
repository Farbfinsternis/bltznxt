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
;     Block, wird es dunkler, je tiefer sie darunter liegt - unter Baeumen
;     etwas, gut zehn Bloecke unter Gestein fast schwarz: Hoehlen sind
;     dunkel wie in Minecraft ohne Fackel (Licht_Tiefe). Gemittelt ueber
;     die vier Zellen an der Ecke, damit Schattenkanten weich auslaufen.
; Was nicht im Chunk-Mesh steckt (Hand, Bruchstuecke, Mitspieler), dunkelt
; Licht_Bei mit derselben Regel am eigenen Ort ab.
; Pflanzen stehen als zwei gekreuzte Flaechen diagonal in ihrer Zelle, wie
; in Minecraft; Gras steht je Zelle etwas versetzt und unterschiedlich hoch,
; damit eine Wiese nicht wie ein Raster aussieht. Sie haben nur das
; Himmelslicht ihrer Zelle, keinen Umgebungsschatten.
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
Dim nz_b(NZ_Y * (HOEHE + 2)) ; Bloecke des Chunks samt Rand
Dim nz_d(NZ_Y * (HOEHE + 2)) ; dasselbe: deckend ja/nein
Dim nz_l(NZ_Y * (HOEHE + 2)) ; dasselbe: Lichtstufe der Fackeln
Dim nz_hm(NZ_Y - 1)         ; oberster deckender Block je Saeule (-1 keiner)
Dim nz_fl(T_ANZAHL - 1)     ; Surface je Textur im entstehenden Mesh

Dim ecke_ao(3)
Dim ecke_hell#(3)
Dim ecke_fk#(3)             ; Fackellicht je Ecke (zweite Texturkoordinate)

Global wasser_pinsel
Global nz_aktuell           ; Mesh, an dem gerade gebaut wird
Global nz_bloecke = 0       ; Statistik: gebaute Flaechen beim letzten Chunk
Global nz_wx0, nz_wz0       ; Weltlage des Chunks, fuer den Versatz der Pflanzen
Global nz_licht_da          ; leuchtet eine Fackel in den Chunk?
Global nz_umgebung# = 1.0   ; Umgebungslicht 0..1 (himmel.bb), fuer Licht_Bei

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
	BrushFX wasser_pinsel, 2 + 16
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
	Return Licht_Tiefe(nz_hm(i Mod NZ_Y) - (i / NZ_Y - 1))
End Function

; Himmelslicht einer Zelle, die d Bloecke unter dem obersten deckenden Block
; ihrer Saeule liegt (d < 0: offener Himmel). Unter einem Baum (d bis 6)
; bleibt es hell genug, ab 14 Bloecken unter Gestein ist es fast schwarz -
; ein Rest bleibt, damit man ohne Fackel wenigstens Umrisse ahnt.
Function Licht_Tiefe#(d)
	If d < 0 Then Return 1.0
	h# = 0.8 - d * 0.055
	If h < 0.04 Then h = 0.04
	Return h
End Function

; Nur das Himmelslicht am Punkt x, y, z (0..1), ohne Tageszeit.
Function Himmel_Bei#(x#, y#, z#)
	bx = Floor(x) : by = Floor(y) : bz = Floor(z)
	For yy = HOEHE - 1 To by + 1 Step -1
		If bl_deckend(Welt_Block(bx, yy, bz)) Then Return Licht_Tiefe(yy - by)
	Next
	Return 1.0
End Function

; Licht am Punkt x, y, z der Welt, fuer Dinge ausserhalb der Chunks, als
; Faktor fuer Vertex- oder Entityfarben (die das Umgebungslicht noch
; dimmt): Himmelslicht, dazu das Fackellicht - durch das Umgebungslicht
; geteilt, damit eine Fackel nachts so hell scheint wie am Tag.
Function Licht_Bei#(x#, y#, z#)
	l# = Himmel_Bei(x, y, z)
	l = l + Fackel_Hell(Fackel_Stufe(Floor(x), Floor(y), Floor(z))) / nz_umgebung
	If l > 1 Then l = 1
	Return l
End Function

; ---- Abdunkeln von Meshes ausserhalb der Chunks ---------------------------
; Hand und Figuren tragen ihre Helligkeit wie die Bloecke in den
; Vertexfarben. Zum Abdunkeln merkt sich Licht_Merken die urspruenglichen
; Farben je Vertex; Licht_Setzen schreibt sie mal einem Faktor zurueck. Eine
; Gruppe ist ein Entity samt allen Kindern (eine Figur).

Type LichtFarbe
	Field gruppe, s, v
	Field r#, g#, b#
End Type

Function Licht_Merken(e, gruppe)
	If EntityClass(e) = "Mesh"
		For n = 1 To CountSurfaces(e)
			s = GetSurface(e, n)
			For v = 0 To CountVertices(s) - 1
				lf.LichtFarbe = New LichtFarbe
				lf\gruppe = gruppe : lf\s = s : lf\v = v
				lf\r = VertexRed(s, v) : lf\g = VertexGreen(s, v) : lf\b = VertexBlue(s, v)
			Next
		Next
	EndIf
	For n = 1 To CountChildren(e)
		Licht_Merken(GetChild(e, n), gruppe)
	Next
End Function

Function Licht_Setzen(gruppe, f#)
	For lf.LichtFarbe = Each LichtFarbe
		If lf\gruppe = gruppe Then VertexColor lf\s, lf\v, lf\r * f, lf\g * f, lf\b * f
	Next
End Function

Function Licht_Vergessen(gruppe)
	For lf.LichtFarbe = Each LichtFarbe
		If lf\gruppe = gruppe Then Delete lf
	Next
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

	Fackeln_Eintragen(c, hmax)

	If c\netz Then FreeEntity c\netz
	If c\wasser Then FreeEntity c\wasser
	c\netz = CreateMesh()
	nz_aktuell = c\netz
	c\wasser = 0
	ws = 0
	For t = 0 To T_ANZAHL - 1 : nz_fl(t) = 0 : Next
	nz_bloecke = 0
	nz_wx0 = c\cx * CH : nz_wz0 = c\cz * CH

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
				ElseIf bl_fackel(b)
					Fackel_Kreuz(b, x, y, z)
				ElseIf b = B_STEINE
					Steine_Klumpen(x, y, z, i)
				ElseIf bl_pflanze(b)
					Pflanzen_Kreuz(b, x, y, z, i)
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

		; Fackellicht genauso gemittelt, mit Richtung und Umgebungsschatten
		If nz_licht_da
			lsum# = nz_l(i + fl_ni(f)) : ln = 1
			If Not s1 Then lsum = lsum + nz_l(i1) : ln = ln + 1
			If Not s2 Then lsum = lsum + nz_l(i2) : ln = ln + 1
			If (Not ec) And (Not (s1 And s2)) Then lsum = lsum + nz_l(i3) : ln = ln + 1
			ecke_fk(k) = Fackel_Hell(lsum / ln) * fl_licht(f) * ao_stufe(ao)
		EndIf
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
	; Der zweite Satz Texturkoordinaten waehlt das Fackellicht auf der
	; Lichtrampe. AddVertex belegt ihn wie das Original mit u, v - ohne diese
	; Zeilen liefe die Rampe quer ueber jede Flaeche.
	If Not nz_licht_da Then ecke_fk(0) = 0 : ecke_fk(1) = 0 : ecke_fk(2) = 0 : ecke_fk(3) = 0
	VertexTexCoords s, v0, ecke_fk(0), 0.5, 0, 1
	VertexTexCoords s, v1, ecke_fk(1), 0.5, 0, 1
	VertexTexCoords s, v2, ecke_fk(2), 0.5, 0, 1
	VertexTexCoords s, v3, ecke_fk(3), 0.5, 0, 1
	; Die Diagonale so legen, dass der Schatten nicht schief verlaeuft.
	If ecke_ao(0) + ecke_ao(2) < ecke_ao(1) + ecke_ao(3)
		AddTriangle s, v0, v1, v3
		AddTriangle s, v1, v2, v3
	Else
		AddTriangle s, v0, v1, v2
		AddTriangle s, v0, v2, v3
	EndIf
End Function

Function Pflanzen_Kreuz(b, x, y, z, i)
	t = bl_tex(b, 0)
	s = nz_fl(t)
	If s = 0
		s = CreateSurface(nz_aktuell, tx_pinsel(t))
		nz_fl(t) = s
	EndIf
	nz_bloecke = nz_bloecke + 2
	hell# = Himmel(i) * 0.9 * 255
	fk# = 0
	If nz_licht_da Then fk = Fackel_Hell(nz_l(i)) * 0.9
	; Versatz aus der Weltlage, nicht aus Rnd - beim Neubau gleich. Gemischt
	; wird mit der Permutationstabelle des Rauschens, ohne Ueberlauf.
	ox# = 0 : oz# = 0 : unten# = 0
	If b = B_HOHESGRAS
		h1 = rs_p(((nz_wx0 + x) And 255) + rs_p((nz_wz0 + z) And 255))
		h2 = rs_p(h1 + 37) : h3 = rs_p(h2 + 91)
		ox = (h1 / 255.0 - 0.5) * 0.4
		oz = (h2 / 255.0 - 0.5) * 0.4
		unten = h3 / 255.0 * 0.25
	EndIf
	a# = 0.15 : e# = 0.85
	For k = 0 To 1
		If k = 0
			x0# = x + a + ox : z0# = z + a + oz : x1# = x + e + ox : z1# = z + e + oz
		Else
			x0 = x + a + ox : z0 = z + e + oz : x1 = x + e + ox : z1 = z + a + oz
		EndIf
		v0 = AddVertex(s, x0, y + 1 - unten, z0, 0, 0)
		v1 = AddVertex(s, x1, y + 1 - unten, z1, 1, 0)
		v2 = AddVertex(s, x1, y - unten, z1, 1, 1)
		v3 = AddVertex(s, x0, y - unten, z0, 0, 1)
		For v = v0 To v3
			VertexColor s, v, hell, hell, hell
			VertexTexCoords s, v, fk, 0.5, 0, 1     ; Fackellicht, s. Block_Flaeche
		Next
		AddTriangle s, v0, v1, v2
		AddTriangle s, v0, v2, v3
	Next
End Function

; Lose Steine: drei kleine Quader aus Steintextur am Boden der Zelle, Lage
; und Groesse aus der Weltlage (wie der Versatz des Grases). Ohne
; Unterseite; die Seiten unterschiedlich hell wie bei den Bloecken.
Function Steine_Klumpen(x, y, z, i)
	t = T_STEIN
	s = nz_fl(t)
	If s = 0
		s = CreateSurface(nz_aktuell, tx_pinsel(t))
		nz_fl(t) = s
	EndIf
	nz_bloecke = nz_bloecke + 1
	hell# = Himmel(i) * 255
	fk# = 0
	If nz_licht_da Then fk = Fackel_Hell(nz_l(i))
	h = rs_p(((nz_wx0 + x) And 255) + rs_p((nz_wz0 + z) And 255))
	For k = 0 To 2
		h = rs_p((h + 53) And 255) : h1 = rs_p((h + 17) And 255) : h2 = rs_p((h + 101) And 255)
		; drei Plaetze im Dreieck, je etwas verschoben
		px# = x + 0.5 + Cos(k * 120 + h) * 0.22
		pz# = z + 0.5 + Sin(k * 120 + h) * 0.22
		gx# = 0.055 + h1 / 255.0 * 0.06
		gz# = 0.05 + h2 / 255.0 * 0.06
		gy# = 0.04 + ((h1 + h2) And 255) / 255.0 * 0.06
		If k = 0 Then gx = gx * 1.3 : gz = gz * 1.3 : gy = gy * 1.3
		Restore nz_kiesel
		For f = 0 To 4
			Read seite_hell#
			v0 = 0
			For c = 0 To 3
				Read ex#, ey#, ez#, u#, v#
				vi = AddVertex(s, px + ex * gx, y + (ey + 1) * gy, pz + ez * gz, (u + h1 / 64.0) * 0.25, (v + h2 / 64.0) * 0.25)
				If c = 0 Then v0 = vi
				VertexColor s, vi, hell * seite_hell, hell * seite_hell, hell * seite_hell
				VertexTexCoords s, vi, fk * seite_hell, 0.5, 0, 1  ; Fackellicht, s. Block_Flaeche
			Next
			AddTriangle s, v0, v0 + 1, v0 + 2
			AddTriangle s, v0, v0 + 2, v0 + 3
		Next
	Next
End Function

; Flaechen eines Klumpens: Helligkeit, dann vier Ecken x y z u v, von aussen
; gesehen im Uhrzeigersinn (oben und die vier Seiten, ohne Unterseite)
.nz_kiesel
Data 1.0,   -1, 1, 1, 0, 0,   1, 1, 1, 1, 0,   1, 1, -1, 1, 1,   -1, 1, -1, 0, 1
Data 0.8,   -1, 1, -1, 0, 0,   1, 1, -1, 1, 0,   1, -1, -1, 1, 1,   -1, -1, -1, 0, 1
Data 0.8,   1, 1, 1, 0, 0,   -1, 1, 1, 1, 0,   -1, -1, 1, 1, 1,   1, -1, 1, 0, 1
Data 0.65,  -1, 1, 1, 0, 0,   -1, 1, -1, 1, 0,   -1, -1, -1, 1, 1,   -1, -1, 1, 0, 1
Data 0.65,  1, 1, -1, 0, 0,   1, 1, 1, 1, 0,   1, -1, 1, 1, 1,   1, -1, -1, 0, 1

; Fackel: dasselbe Kreuz, schmal genug fuer einen Stab. An der Wand steht
; der Fuss an der Wand und etwas hoeher, die Spitze neigt sich in den Raum.
Function Fackel_Kreuz(b, x, y, z)
	t = T_FACKEL
	s = nz_fl(t)
	If s = 0
		s = CreateSurface(nz_aktuell, tx_pinsel(t))
		nz_fl(t) = s
	EndIf
	nz_bloecke = nz_bloecke + 2
	wx# = fk_wx(b) : wz# = fk_wz(b)
	wand = (bl_fackel(b) = 2)
	oben# = y + 1 + 0.15 * wand : unten# = y + 0.15 * wand
	a# = 0.2 : e# = 0.8
	For k = 0 To 1
		If k = 0
			x0# = x + a : z0# = z + a : x1# = x + e : z1# = z + e
		Else
			x0 = x + a : z0 = z + e : x1 = x + e : z1 = z + a
		EndIf
		; oben, dann unten; an der Wand steht der Fuss an der Wand (0,4 vom
		; Mittelpunkt), die Spitze nur 0,12 - die Fackel neigt sich weg
		v0 = AddVertex(s, x0 + wx * 0.12, oben, z0 + wz * 0.12, 0, 0)
		v1 = AddVertex(s, x1 + wx * 0.12, oben, z1 + wz * 0.12, 1, 0)
		v2 = AddVertex(s, x1 + wx * 0.4, unten, z1 + wz * 0.4, 1, 1)
		v3 = AddVertex(s, x0 + wx * 0.4, unten, z0 + wz * 0.4, 0, 1)
		For v = v0 To v3
			VertexColor s, v, 255, 255, 255
		Next
		AddTriangle s, v0, v1, v2
		AddTriangle s, v0, v2, v3
	Next
End Function

; Lichtstufe 0..14 als Helligkeit 0..1, wie die Kurve in Minecraft: nahe der
; Fackel hell, nach wenigen Bloecken schon schwach. Mit der mittleren
; Helligkeitseinstellung von Minecraft (Gamma 0,5) - die dunkelste liess
; einen Fackelschein kaum zwei Bloecke weit reichen.
Function Fackel_Hell#(l#)
	x# = l / 15.0
	f# = x / (4 - 3 * x)
	g# = 1 - f
	Return (f + 1 - g * g * g * g) * 0.5
End Function

; Beim Bauen: die Lichtbanks der Fackeln, die in den Chunk samt Rand
; reichen, ins Feld nz_l eintragen (die hellste Fackel zaehlt).
Function Fackeln_Eintragen(c.Chunk, hmax)
	nz_licht_da = False
	x0 = c\cx * CH - 1 : z0 = c\cz * CH - 1     ; Weltlage der Feldzelle X = 0, Z = 0
	For f.Fackel = Each Fackel
		If f\x + FK_R >= x0 And f\x - FK_R <= x0 + 17 And f\z + FK_R >= z0 And f\z - FK_R <= z0 + 17
			If Not nz_licht_da
				For i = 0 To NZ_Y * (hmax + 4) - 1
					nz_l(i) = 0
				Next
				nz_licht_da = True
			EndIf
			xa = f\x - FK_R : If xa < x0 Then xa = x0
			xb = f\x + FK_R : If xb > x0 + 17 Then xb = x0 + 17
			za = f\z - FK_R : If za < z0 Then za = z0
			zb = f\z + FK_R : If zb > z0 + 17 Then zb = z0 + 17
			ya = f\y - FK_R : If ya < 0 Then ya = 0
			yb = f\y + FK_R : If yb > hmax + 2 Then yb = hmax + 2
			For wy = ya To yb
				For wz = za To zb
					o = (wz - f\z + FK_R) * FK_N + (wy - f\y + FK_R) * FK_N * FK_N - f\x + FK_R
					i = (wz - z0) * NZ_Z + (wy + 1) * NZ_Y - x0
					For wx = xa To xb
						v = PeekByte(f\licht, o + wx)
						If v > nz_l(i + wx) Then nz_l(i + wx) = v
					Next
				Next
			Next
		EndIf
	Next
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
