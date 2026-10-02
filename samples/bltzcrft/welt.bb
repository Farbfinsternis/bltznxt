; BLTZCRFT - Welt
;
; Die Welt besteht aus Chunks zu 16 x 128 x 16 Bloecken, jeder Block ein Byte
; in einer Bank. Die geladenen Chunks liegen in einem Ring aus 32 x 32
; Plaetzen: Chunk (cx, cz) gehoert auf Platz (cx And 31, cz And 31). Weil die
; Sichtweite kleiner ist als der Ring, teilen sich nie zwei geladene Chunks
; einen Platz.
;
; Erzeugt wird ein Chunk allein aus der Saat und seinen Koordinaten, in dieser
; Reihenfolge:
;   1. Gelaende: Hoehe je Saeule aus Rauschen - Kontinente und Ozean, Huegel,
;      Gebirge, Senken fuer Seen. Unter der Oberflaeche Erde bzw. Sand, darunter
;      Stein, ganz unten Grundgestein. Bis zum Meeresspiegel steht Wasser.
;   2. Hoehlen: zwei 3D-Rauschfelder, wo beide nahe null sind, liegt ein
;      Gang ("Spaghetti"); tief unten zusaetzlich grosse Hallen.
;   3. Erze: Kohle und Eisen in Klumpen, nur im Stein.
;   4. Tuempel: manche Chunks bekommen auf flachem Land eine Mulde mit Wasser.
;   5. Baeume auf Gras.
; Tuempel und Baeume bleiben innerhalb ihres Chunks, so braucht kein Chunk
; beim Erzeugen seine Nachbarn.

Const CH = 16               ; Kantenlaenge eines Chunks
Const HOEHE = 128           ; Hoehe der Welt
Const CH_BYTES = 32768      ; 16 * 16 * 128
Const RING = 32
Const MEER = 48             ; der oberste Wasserblock des Ozeans

Type Chunk
	Field cx, cz
	Field daten             ; Bank mit den Bloecken
	Field netz              ; Mesh der festen Bloecke (0 = noch keins)
	Field wasser            ; Mesh des Wassers
	Field veraltet          ; Mesh muss neu gebaut werden
	Field hmax              ; hoechster Block, der nicht Luft ist
End Type

Dim ch_ring.Chunk(RING - 1, RING - 1)

Global welt_saat = 1
Global welt_chunks = 0      ; geladene Chunks

; Hilfsfelder beim Erzeugen (Saeulen -1..16 -> Index 0..17)
Dim gen_h(17, 17)
Dim gen_oben(15, 15)        ; Oberflaechenblock der Saeule
; Hoehlenrauschen auf einem groben Gitter (alle 4 Bloecke), dazwischen linear
Dim gen_c1#(4, 4, 32)
Dim gen_c2#(4, 4, 32)
Dim gen_c3#(4, 4, 32)
Dim gen_s1#(32)              ; dasselbe, fuer eine Saeule in der Ebene interpoliert
Dim gen_s2#(32)
Dim gen_s3#(32)

Function Welt_Saat(saat)
	welt_saat = saat
	Rauschen_Saat(saat)
End Function

; ---- Zugriff -----------------------------------------------------------

Function Chunk_Holen.Chunk(cx, cz)
	c.Chunk = ch_ring(cx And (RING - 1), cz And (RING - 1))
	If c <> Null
		If c\cx = cx And c\cz = cz Then Return c
	EndIf
	Return Null
End Function

Function Ofs(x, y, z)
	Return (y Shl 8) Or (z Shl 4) Or x
End Function

; Block an einer Weltstelle. Unter der Welt liegt Grundgestein; was in
; einem nicht geladenen Chunk liegt, gilt als Stein, damit niemand hineinfaellt.
Function Welt_Block(x, y, z)
	If y < 0 Then Return B_GRUND
	If y >= HOEHE Then Return B_LUFT
	c.Chunk = Chunk_Holen(x Sar 4, z Sar 4)
	If c = Null Then Return B_STEIN
	Return PeekByte(c\daten, Ofs(x And 15, y, z And 15))
End Function

Function Welt_Setzen(x, y, z, b)
	If y < 1 Or y >= HOEHE Then Return False
	c.Chunk = Chunk_Holen(x Sar 4, z Sar 4)
	If c = Null Then Return False
	lx = x And 15 : lz = z And 15
	PokeByte c\daten, Ofs(lx, y, lz), b
	If y > c\hmax Then c\hmax = y
	c\veraltet = True
	; Am Rand sehen auch die Nachbarn den Block (Flaechen, Schatten).
	For dz = -1 To 1
		For dx = -1 To 1
			If (dx = -1 And lx = 0) Or (dx = 1 And lx = 15) Or dx = 0
				If (dz = -1 And lz = 0) Or (dz = 1 And lz = 15) Or dz = 0
					n.Chunk = Chunk_Holen(c\cx + dx, c\cz + dz)
					If n <> Null Then n\veraltet = True
				EndIf
			EndIf
		Next
	Next
	Return True
End Function

; ---- Laden und Entladen --------------------------------------------------

Function Chunk_Neu.Chunk(cx, cz)
	alt.Chunk = ch_ring(cx And (RING - 1), cz And (RING - 1))
	If alt <> Null Then Chunk_Weg(alt)
	c.Chunk = New Chunk
	c\cx = cx : c\cz = cz
	c\daten = CreateBank(CH_BYTES)
	c\veraltet = True
	ch_ring(cx And (RING - 1), cz And (RING - 1)) = c
	welt_chunks = welt_chunks + 1
	Chunk_Erzeugen(c)
	Return c
End Function

Function Chunk_Weg(c.Chunk)
	If c\netz Then FreeEntity c\netz
	If c\wasser Then FreeEntity c\wasser
	FreeBank c\daten
	ch_ring(c\cx And (RING - 1), c\cz And (RING - 1)) = Null
	Delete c
	welt_chunks = welt_chunks - 1
End Function

; ---- Gelaende ------------------------------------------------------------

Function Klemmen#(x#, a#, b#)
	If x < a Then Return a
	If x > b Then Return b
	Return x
End Function

; Wie viel "Land" eine Stelle ist: 0 im Ozean, 1 im Binnenland.
Function Kontinent#(x#, z#)
	Return Fbm2(x / 600.0, z / 600.0, 4, 0) + 0.1
End Function

Function Gelaende_Hoehe(x#, z#)
	Local h#
	k# = Kontinent(x, z)
	If k < 0
		h# = MEER + 2 + k * 60
	Else
		h = MEER + 2 + k * 26
	EndIf
	land# = Klemmen((k + 0.05) / 0.3, 0, 1)

	; Huegel, im Binnenland kraeftiger
	h = h + Fbm2(x / 110.0, z / 110.0, 4, 40) * (3 + 9 * land)

	; Gebirge: Grate aus 1 - |Rauschen|, nur wo die Gebirgsmaske es zulaesst
	maske# = Klemmen((Fbm2(x / 380.0, z / 380.0, 2, 120) - 0.05) * 2.2, 0, 1)
	If maske > 0
		grat# = 1 - Abs(Fbm2(x / 160.0, z / 160.0, 3, 70))
		h = h + grat * grat * grat * 62 * maske * land
	EndIf

	; Senken: im flachen Land laufen sie unter den Meeresspiegel - ein See
	s# = Fbm2(x / 140.0, z / 140.0, 3, 160)
	If s > 0.35 Then h = h - (s - 0.35) * 80 * land

	If h < 6 Then h = 6
	If h > HOEHE - 12 Then h = HOEHE - 12
	Return Int(Floor(h))
End Function

Function Schneegrenze(x#, z#)
	Return 92 + Int(Rauschen2(x / 40.0, z / 40.0, 300) * 8)
End Function

; Oberflaechenblock einer Saeule; steil = groesster Hoehenunterschied zum
; Nachbarn.
Function Oberflaeche(x#, z#, h, steil)
	If h < MEER
		If MEER - h <= 3 Then Return B_SAND
		r# = Rauschen2(x / 24.0, z / 24.0, 210)
		If r > 0.2 Then Return B_KIES
		If r < -0.25 Then Return B_TON
		Return B_SAND
	EndIf
	If h <= MEER + 1
		If Rauschen2(x / 50.0, z / 50.0, 230) > -0.25 Then Return B_SAND
	EndIf
	If steil >= 4 And h > MEER + 6 Then Return B_STEIN
	If h >= Schneegrenze(x, z)
		If steil >= 3 Then Return B_STEIN
		Return B_SCHNEE
	EndIf
	If h > 104 Then Return B_STEIN
	Return B_GRAS
End Function

Function Fuellung(oben)
	Select oben
	Case B_GRAS, B_SCHNEE : Return B_ERDE
	Case B_SAND : Return B_SAND
	Case B_KIES : Return B_KIES
	Case B_TON : Return B_TON
	End Select
	Return B_STEIN
End Function

; ---- Erzeugen ------------------------------------------------------------

Function Chunk_Erzeugen(c.Chunk)
	bank = c\daten
	x0 = c\cx * CH : z0 = c\cz * CH
	; Zufall nur aus Saat und Lage, nie aus der Reihenfolge des Ladens
	SeedRnd welt_saat + c\cx * 7919 + c\cz * 6271

	; Hoehen mit einem Rand, fuer die Steilheit
	For i = 0 To 17
		For k = 0 To 17
			gen_h(i, k) = Gelaende_Hoehe(x0 + i - 1, z0 + k - 1)
		Next
	Next

	hmax = 0
	For z = 0 To 15
		For x = 0 To 15
			h = gen_h(x + 1, z + 1)
			steil = 0
			d = Abs(gen_h(x, z + 1) - h) : If d > steil Then steil = d
			d = Abs(gen_h(x + 2, z + 1) - h) : If d > steil Then steil = d
			d = Abs(gen_h(x + 1, z) - h) : If d > steil Then steil = d
			d = Abs(gen_h(x + 1, z + 2) - h) : If d > steil Then steil = d
			oben = Oberflaeche(x0 + x, z0 + z, h, steil)
			gen_oben(x, z) = oben
			fuell = Fuellung(oben)
			tief = 3 + (h And 1)

			PokeByte bank, Ofs(x, 0, z), B_GRUND
			For y = 1 To h
				If y <= 3 And Rand(0, y) = 0
					b = B_GRUND
				ElseIf y = h
					b = oben
				ElseIf y > h - tief
					b = fuell
				ElseIf oben = B_SAND And y > h - tief - 3
					b = B_SANDSTEIN
				Else
					b = B_STEIN
				EndIf
				PokeByte bank, Ofs(x, y, z), b
			Next
			For y = h + 1 To MEER
				PokeByte bank, Ofs(x, y, z), B_WASSER
			Next
			If h > hmax Then hmax = h
		Next
	Next
	If hmax < MEER Then hmax = MEER

	Hoehlen_Graben(c, x0, z0)
	SeedRnd welt_saat + c\cx * 7919 + c\cz * 6271
	Erze_Setzen(bank)
	Tuempel_Graben(bank)
	hmax = Baeume_Pflanzen(bank, x0, z0, hmax)
	c\hmax = hmax
End Function

Function Hoehlen_Graben(c.Chunk, x0, z0)
	bank = c\daten
	; grobes Gitter, nur so hoch wie das Gelaende reicht
	jmax = 0
	For k = 1 To 16
		For i = 1 To 16
			If gen_h(i, k) Shr 2 > jmax Then jmax = gen_h(i, k) Shr 2
		Next
	Next
	If jmax > 31 Then jmax = 31
	For i = 0 To 4
		For k = 0 To 4
			wx# = x0 + i * 4 : wz# = z0 + k * 4
			For j = 0 To jmax + 1
				wy# = j * 4
				gen_c1(i, k, j) = Rauschen3(wx / 30.0, wy / 20.0, wz / 30.0)
				gen_c2(i, k, j) = Rauschen3(wx / 30.0 + 61.3, wy / 20.0 + 17.9, wz / 30.0 + 33.1)
				If j <= 10 Then gen_c3(i, k, j) = Rauschen3(wx / 64.0 + 120.5, wy / 32.0, wz / 64.0 + 77.7)
			Next
		Next
	Next

	For z = 0 To 15
		k = z Shr 2 : fz# = (z And 3) / 4.0
		For x = 0 To 15
			i = x Shr 2 : fx# = (x And 3) / 4.0
			h = gen_h(x + 1, z + 1)
			; Neben und unter Wasser bleibt eine Decke von fuenf Bloecken, sonst
			; liefe ein Gang voll - Wasser fliesst hier nicht.
			bis = h
			tief = h
			For dz = 0 To 2
				For dx = 0 To 2
					If gen_h(x + dx, z + dz) < tief Then tief = gen_h(x + dx, z + dz)
				Next
			Next
			If tief <= MEER + 1 And tief - 5 < bis Then bis = tief - 5

			; In der Ebene einmal je Saeule interpolieren, in der Hoehe je Block
			For j = 0 To (bis Shr 2) + 1
				a# = gen_c1(i, k, j) + (gen_c1(i + 1, k, j) - gen_c1(i, k, j)) * fx
				b# = gen_c1(i, k + 1, j) + (gen_c1(i + 1, k + 1, j) - gen_c1(i, k + 1, j)) * fx
				gen_s1(j) = a + (b - a) * fz
				a = gen_c2(i, k, j) + (gen_c2(i + 1, k, j) - gen_c2(i, k, j)) * fx
				b = gen_c2(i, k + 1, j) + (gen_c2(i + 1, k + 1, j) - gen_c2(i, k + 1, j)) * fx
				gen_s2(j) = a + (b - a) * fz
				If j <= 10
					a = gen_c3(i, k, j) + (gen_c3(i + 1, k, j) - gen_c3(i, k, j)) * fx
					b = gen_c3(i, k + 1, j) + (gen_c3(i + 1, k + 1, j) - gen_c3(i, k + 1, j)) * fx
					gen_s3(j) = a + (b - a) * fz
				EndIf
			Next

			For y = 1 To bis
				j = y Shr 2 : fy# = (y And 3) / 4.0
				frei = False
				n1# = gen_s1(j) + (gen_s1(j + 1) - gen_s1(j)) * fy
				If Abs(n1) < 0.06
					n2# = gen_s2(j) + (gen_s2(j + 1) - gen_s2(j)) * fy
					frei = (n1 * n1 + n2 * n2 < 0.0035)
				EndIf
				If (Not frei) And y < 36
					n3# = gen_s3(j) + (gen_s3(j + 1) - gen_s3(j)) * fy
					frei = (n3 > 0.36 - (36 - y) * 0.004)
				EndIf
				If frei
					o = Ofs(x, y, z)
					If PeekByte(bank, o) <> B_WASSER Then PokeByte bank, o, B_LUFT
				EndIf
			Next
		Next
	Next
End Function

Function Erze_Setzen(bank)
	For n = 1 To 18
		Erz_Klumpen(bank, B_KOHLE, Rand(5, 90), Rand(4, 9))
	Next
	For n = 1 To 9
		Erz_Klumpen(bank, B_EISEN, Rand(5, 50), Rand(3, 6))
	Next
End Function

Function Erz_Klumpen(bank, art, y, groesse)
	x = Rand(1, 14) : z = Rand(1, 14)
	For i = 1 To groesse
		If y >= 1 And y < HOEHE
			o = Ofs(x, y, z)
			If PeekByte(bank, o) = B_STEIN Then PokeByte bank, o, art
		EndIf
		Select Rand(0, 5)
		Case 0 : If x < 15 Then x = x + 1
		Case 1 : If x > 0 Then x = x - 1
		Case 2 : If z < 15 Then z = z + 1
		Case 3 : If z > 0 Then z = z - 1
		Case 4 : y = y + 1
		Case 5 : y = y - 1
		End Select
	Next
End Function

; Eine Mulde mit Wasser auf flachem Land, ueber dem Meeresspiegel.
Function Tuempel_Graben(bank)
	If Rand(0, 5) <> 0 Then Return
	mx = Rand(6, 9) : mz = Rand(6, 9)
	rx = Rand(3, 5) : rz = Rand(3, 5)

	; Spiegel = tiefste Stelle der Mulde samt Rand; flach genug?
	spiegel = 999 : hoch = 0
	For z = mz - rz - 1 To mz + rz + 1
		For x = mx - rx - 1 To mx + rx + 1
			h = gen_h(x + 1, z + 1)
			If h < spiegel Then spiegel = h
			If h > hoch Then hoch = h
			If gen_oben(x, z) <> B_GRAS Then Return
		Next
	Next
	If spiegel <= MEER + 2 Or hoch - spiegel > 3 Then Return

	For z = mz - rz To mz + rz
		For x = mx - rx To mx + rx
			dx# = (x - mx) / Float(rx) : dz# = (z - mz) / Float(rz)
			d# = dx * dx + dz * dz
			If d < 1
				tiefe = 1 + Int((1 - d) * 3)
				boden = spiegel - tiefe
				If Rand(0, 2) = 0 Then b = B_TON Else b = B_SAND
				PokeByte bank, Ofs(x, boden, z), b
				For y = boden + 1 To spiegel
					PokeByte bank, Ofs(x, y, z), B_WASSER
				Next
				For y = spiegel + 1 To gen_h(x + 1, z + 1)
					PokeByte bank, Ofs(x, y, z), B_LUFT
				Next
				gen_oben(x, z) = B_WASSER
			EndIf
		Next
	Next

	; Grenzt eine Hoehle an, liefe das Wasser hinein: Rand mit Erde schliessen.
	For z = mz - rz To mz + rz
		For x = mx - rx To mx + rx
			If gen_oben(x, z) = B_WASSER
				For y = spiegel - 4 To spiegel
					For r = 0 To 4
						nx = x : nz = z : ny = y
						Select r
						Case 0 : nx = x + 1
						Case 1 : nx = x - 1
						Case 2 : nz = z + 1
						Case 3 : nz = z - 1
						Case 4 : ny = y - 1
						End Select
						If PeekByte(bank, Ofs(nx, ny, nz)) = B_LUFT And (gen_oben(nx, nz) <> B_WASSER Or ny < y)
							PokeByte bank, Ofs(nx, ny, nz), B_ERDE
						EndIf
					Next
				Next
			EndIf
		Next
	Next
End Function

Function Baeume_Pflanzen(bank, x0, z0, hmax)
	For z = 2 To 13
		For x = 2 To 13
			If gen_oben(x, z) = B_GRAS
				h = gen_h(x + 1, z + 1)
				; Dichte: Waelder und offene Wiesen
				wald# = Fbm2((x0 + x) / 160.0, (z0 + z) / 160.0, 2, 260)
				chance# = Klemmen(0.004 + wald * 0.07, 0.002, 0.06)
				If h > MEER + 1 And Rnd(0, 1) < chance
					If PeekByte(bank, Ofs(x, h, z)) = B_GRAS And PeekByte(bank, Ofs(x, h + 1, z)) = B_LUFT
						t = Baum(bank, x, h + 1, z)
						If t > hmax Then hmax = t
					EndIf
				EndIf
			EndIf
		Next
	Next
	Return hmax
End Function

Function Baum(bank, x, y, z)
	stamm = Rand(4, 6)
	PokeByte bank, Ofs(x, y - 1, z), B_ERDE
	spitze = y + stamm - 1
	For ly = spitze - 2 To spitze + 1
		If ly >= spitze Then r = 1 Else r = 2
		For dz = -r To r
			For dx = -r To r
				ecke = (Abs(dx) = r And Abs(dz) = r)
				If (Not ecke) Or (r = 2 And Rand(0, 1) = 0 And ly < spitze - 1)
					If ly < HOEHE
						o = Ofs(x + dx, ly, z + dz)
						If PeekByte(bank, o) = B_LUFT Then PokeByte bank, o, B_LAUB
					EndIf
				EndIf
			Next
		Next
	Next
	For ly = y To spitze
		PokeByte bank, Ofs(x, ly, z), B_STAMM
	Next
	Return spitze + 1
End Function
