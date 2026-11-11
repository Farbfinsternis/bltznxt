; BLTZCRFT - Welt
;
; Die Welt besteht aus Chunks zu 16 x 256 x 16 Bloecken, jeder Block ein Byte
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
;   2. Hoehlen aus 3D-Rauschen, wie in Minecraft seit 1.18: Kaesehoehlen
;      bis zu kilometerweiten Gewoelben, Spaghetti-Gaenge von eng bis
;      breit (die Eingaenge) und winzige Nudel-Gaenge (Hoehlen_Graben).
;   3. Erze: Kohle und Eisen in Klumpen, nur im Stein.
;   4. Tuempel: manche Chunks bekommen auf flachem Land eine Mulde mit Wasser.
;   5. Baeume auf Gras.
; Tuempel und Baeume bleiben innerhalb ihres Chunks, so braucht kein Chunk
; beim Erzeugen seine Nachbarn.

Const CH = 16               ; Kantenlaenge eines Chunks
Const HOEHE = 256           ; Hoehe der Welt
Const CH_BYTES = 65536      ; 16 * 16 * 256
Const RING = 32
Const MEER = 112            ; der oberste Wasserblock des Ozeans - darunter
                            ; ueber 100 Bloecke Gestein fuer die Hoehlen

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
Const GEN_J = 65             ; Gitterpunkte in der Hoehe (256 / 4 + 1)
Dim gen_c1#(4, 4, GEN_J)     ; Kaese
Dim gen_c2#(4, 4, GEN_J)     ; Spaghetti, zwei Felder
Dim gen_c3#(4, 4, GEN_J)
Dim gen_c4#(4, 4, GEN_J)     ; Spaghetti-Dicke
Dim gen_c5#(4, 4, GEN_J)     ; Nudeln, zwei Felder
Dim gen_c6#(4, 4, GEN_J)
Dim gen_s1#(GEN_J)           ; dasselbe, fuer eine Saeule in der Ebene interpoliert
Dim gen_s2#(GEN_J)
Dim gen_s3#(GEN_J)
Dim gen_s4#(GEN_J)
Dim gen_s5#(GEN_J)
Dim gen_s6#(GEN_J)
Dim gen_gew#(4, 4)           ; Gewoelbe-Gebiet je Gitterspalte (2D)
Dim gen_nud#(4, 4)           ; Nudel-Gebiet je Gitterspalte (2D)

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

; ---- Aenderungen -----------------------------------------------------------
;
; Jeder abgebaute oder gesetzte Block kommt in eine Liste. Ein Chunk, der zu
; weit weg war, entsteht beim Zurueckkommen wieder aus der Saat und bekommt
; seine Aenderungen dann neu. Im Mehrspieler kommen auch die Bloecke der
; anderen hierher, auch fuer Chunks, die hier noch gar nicht geladen sind.
; Je Stelle steht nur der letzte Block in der Liste.

Type Aenderung
	Field x, y, z, b
End Type

Function Welt_Aendern(x, y, z, b)
	If y < 1 Or y >= HOEHE Then Return False
	alt = Welt_Block(x, y, z)
	gefunden = False
	For a.Aenderung = Each Aenderung
		If a\x = x And a\y = y And a\z = z Then a\b = b : gefunden = True : Exit
	Next
	If Not gefunden
		a.Aenderung = New Aenderung
		a\x = x : a\y = y : a\z = z : a\b = b
	EndIf
	Welt_Setzen(x, y, z, b)
	Fackeln_Aendern(x, y, z, alt, b)
	Return True
End Function

; ---- Fackeln ---------------------------------------------------------------
;
; Licht wie in Minecraft: eine Fackel hat Stufe 14, jeder Schritt zur
; Nachbarzelle kostet eine Stufe; deckende Bloecke halten es auf, um Ecken
; kommt es herum. Je Fackel liegt das Ergebnis in einer Bank fuer den
; Wuerfel von 29 Zellen Kantenlaenge um sie herum (Ausbreitung in die
; Breite). Neu gerechnet wird nur, wenn sich in diesem Wuerfel ein Block
; zwischen deckend und offen aendert oder ein Chunk darin neu geladen wird;
; dann werden die betroffenen Chunks neu gebaut (netz.bb traegt die Banks
; beim Bauen ein). Ein nicht geladener Chunk zaehlt wie Stein - es scheint
; nichts hinein, bis er da ist.

Const FK_R = 14                 ; Lichtstufe der Fackel = Reichweite
Const FK_N = 29                 ; Kantenlaenge des Wuerfels
Const FK_VOLUMEN = 24389        ; 29 * 29 * 29

Type Fackel
	Field x, y, z
	Field licht                 ; Bank: Lichtstufe je Zelle, x + z*29 + y*841
	Field hand                  ; die Fackel in der Hand (hand.bb), kein Block
End Type

Dim fk_q(FK_VOLUMEN - 1)

Function Fackeln_Aendern(x, y, z, alt, neu)
	If bl_fackel(alt)
		For f.Fackel = Each Fackel
			If f\x = x And f\y = y And f\z = z And (Not f\hand)
				FreeBank f\licht
				Delete f
				Fackel_Chunks_Veralten(x, z)
				Exit
			EndIf
		Next
	EndIf
	If bl_fackel(neu)
		f.Fackel = New Fackel
		f\x = x : f\y = y : f\z = z
		f\licht = CreateBank(FK_VOLUMEN)
		Fackel_Leuchten(f)
		Fackel_Chunks_Veralten(x, z)
	ElseIf bl_deckend(alt) <> bl_deckend(neu)
		For f.Fackel = Each Fackel
			If Abs(f\x - x) <= FK_R And Abs(f\y - y) <= FK_R And Abs(f\z - z) <= FK_R
				Fackel_Leuchten(f)
				Fackel_Chunks_Veralten(f\x, f\z)
			EndIf
		Next
	EndIf
End Function

; Ein Chunk ist neu da: Fackeln, deren Licht hineinreicht, neu rechnen.
Function Fackeln_Chunk_Neu(c.Chunk)
	x0 = c\cx * CH : z0 = c\cz * CH
	For f.Fackel = Each Fackel
		If f\x + FK_R >= x0 And f\x - FK_R < x0 + CH And f\z + FK_R >= z0 And f\z - FK_R < z0 + CH
			Fackel_Leuchten(f)
			Fackel_Chunks_Veralten(f\x, f\z)
		EndIf
	Next
End Function

Function Fackel_Chunks_Veralten(x, z)
	For cz = (z - FK_R - 1) Sar 4 To (z + FK_R + 1) Sar 4
		For cx = (x - FK_R - 1) Sar 4 To (x + FK_R + 1) Sar 4
			c.Chunk = Chunk_Holen(cx, cz)
			If c <> Null Then c\veraltet = True
		Next
	Next
End Function

; Ausbreitung in die Breite: jede Zelle kommt einmal in die Schlange, mit
; der hoechsten Stufe, die sie erreicht.
Function Fackel_Leuchten(f.Fackel)
	bank = f\licht
	For i = 0 To FK_VOLUMEN - 1
		PokeByte bank, i, 0
	Next
	mitte = FK_R + FK_R * FK_N + FK_R * FK_N * FK_N
	PokeByte bank, mitte, FK_R
	fk_q(0) = mitte : kopf = 0 : ende = 1
	While kopf < ende
		i = fk_q(kopf) : kopf = kopf + 1
		l = PeekByte(bank, i) - 1
		If l > 0
			lx = i Mod FK_N : lz = (i / FK_N) Mod FK_N : ly = i / (FK_N * FK_N)
			For k = 0 To 5
				nx = lx : ny = ly : nz = lz
				Select k
				Case 0 : nx = lx + 1
				Case 1 : nx = lx - 1
				Case 2 : ny = ly + 1
				Case 3 : ny = ly - 1
				Case 4 : nz = lz + 1
				Case 5 : nz = lz - 1
				End Select
				If nx >= 0 And nx < FK_N And ny >= 0 And ny < FK_N And nz >= 0 And nz < FK_N
					j = nx + nz * FK_N + ny * FK_N * FK_N
					If PeekByte(bank, j) < l
						If Not bl_deckend(Welt_Block(f\x + nx - FK_R, f\y + ny - FK_R, f\z + nz - FK_R))
							PokeByte bank, j, l
							fk_q(ende) = j : ende = ende + 1
						EndIf
					EndIf
				EndIf
			Next
		EndIf
	Wend
End Function

; Lichtstufe der Fackeln an einer Weltzelle (0..14).
Function Fackel_Stufe(x, y, z)
	m = 0
	For f.Fackel = Each Fackel
		dx = x - f\x : dy = y - f\y : dz = z - f\z
		If Abs(dx) <= FK_R And Abs(dy) <= FK_R And Abs(dz) <= FK_R
			v = PeekByte(f\licht, (dx + FK_R) + (dz + FK_R) * FK_N + (dy + FK_R) * FK_N * FK_N)
			If v > m Then m = v
		EndIf
	Next
	Return m
End Function

; Gleich nach dem Erzeugen: die Nachbarn sind dann noch nicht gebaut, ein
; Block am Rand braucht sie nicht neu zu bauen.
Function Aenderungen_Anwenden(c.Chunk)
	For a.Aenderung = Each Aenderung
		If (a\x Sar 4) = c\cx And (a\z Sar 4) = c\cz
			PokeByte c\daten, Ofs(a\x And 15, a\y, a\z And 15), a\b
			If a\y > c\hmax Then c\hmax = a\y
		EndIf
	Next
End Function

; Eine andere Welt: alle Chunks weg, neue Saat. Die Aenderungen bleiben -
; sie gehoeren schon zur neuen Welt (Beitritt im Mehrspieler).
Function Welt_Neu(saat)
	c.Chunk = First Chunk
	While c <> Null
		n.Chunk = After c
		Chunk_Weg(c)
		c = n
	Wend
	Welt_Saat(saat)
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
	Aenderungen_Anwenden(c)
	Fackeln_Chunk_Neu(c)
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
	Return MEER + 44 + Int(Rauschen2(x / 40.0, z / 40.0, 300) * 8)
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
	If h > MEER + 56 Then Return B_STEIN
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
	hmax = Pflanzen_Saeen(bank, x0, z0, hmax)
	c\hmax = hmax
End Function

; Hoehlen wie in Minecraft seit 1.18, drei Arten aus 3D-Rauschen:
;  - Kaesehoehlen: wo ein grobes, in der Hoehe gestrecktes Rauschen ueber
;    einer Schwelle liegt, ist Luft. Die Schwelle senkt ein 2D-Rauschen mit
;    einer Wellenlaenge von ueber einem Kilometer: in diesen Gebieten werden
;    aus einzelnen Blasen zusammenhaengende Gewoelbe, Hunderte Bloecke weit
;    und Dutzende hoch, mit Pfeilern dazwischen. Nahe der Oberflaeche und
;    ueber dem Grundgestein bleibt es geschlossen, damit Decke und Boden
;    entstehen.
;  - Spaghetti: zwei 3D-Rauschfelder, wo beide nahe null sind, laeuft ein
;    Gang. Sein Querschnitt schwankt mit einem dritten Rauschen von einem
;    engen Schlupf bis zu einem breiten Tunnel. Sie duerfen bis an die
;    Oberflaeche - das sind die Hoehleneingaenge.
;  - Nudeln: dasselbe mit hoeherer Frequenz und winzigem Querschnitt, nur in
;    manchen Gebieten - verwinkelte Kriechgaenge.
; Neben und unter Wasser bleibt eine Decke von fuenf Bloecken, sonst liefe
; eine Hoehle voll - Wasser fliesst hier nicht.
;
; Das Rauschen liegt auf einem groben Gitter (alle 4 Bloecke); je Saeule wird
; einmal in der Ebene interpoliert, je Block nur noch in der Hoehe. Das
; zweite Feld eines Gangs wird nur befragt, wenn das erste nahe null liegt.
Function Hoehlen_Graben(c.Chunk, x0, z0)
	bank = c\daten
	; grobes Gitter, nur so hoch wie das Gelaende reicht
	jmax = 0
	For k = 1 To 16
		For i = 1 To 16
			If gen_h(i, k) Shr 2 > jmax Then jmax = gen_h(i, k) Shr 2
		Next
	Next
	If jmax > GEN_J - 2 Then jmax = GEN_J - 2
	For i = 0 To 4
		For k = 0 To 4
			wx# = x0 + i * 4 : wz# = z0 + k * 4
			gen_gew(i, k) = Fbm2(wx / 1100.0, wz / 1100.0, 2, 520)
			gen_nud(i, k) = Rauschen2(wx / 220.0, wz / 220.0, 610)
			For j = 0 To jmax + 1
				wy# = j * 4
				gen_c1(i, k, j) = Rauschen3(wx / 90.0, wy / 48.0, wz / 90.0) * 0.7 + Rauschen3(wx / 34.0 + 7.7, wy / 22.0, wz / 34.0) * 0.3
				gen_c2(i, k, j) = Rauschen3(wx / 52.0, wy / 36.0, wz / 52.0 + 11.3)
				gen_c3(i, k, j) = Rauschen3(wx / 52.0 + 61.3, wy / 36.0 + 17.9, wz / 52.0 + 33.1)
				gen_c4(i, k, j) = Rauschen3(wx / 140.0 + 200.1, wy / 70.0, wz / 140.0 + 5.5)
				If gen_nud(i, k) > -0.05
					gen_c5(i, k, j) = Rauschen3(wx / 22.0 + 300.7, wy / 16.0, wz / 22.0 + 2.2)
					gen_c6(i, k, j) = Rauschen3(wx / 22.0 + 41.9, wy / 16.0 + 80.3, wz / 22.0 + 9.1)
				Else
					gen_c5(i, k, j) = 1 : gen_c6(i, k, j) = 1
				EndIf
			Next
		Next
	Next

	For z = 0 To 15
		k = z Shr 2 : fz# = (z And 3) / 4.0
		For x = 0 To 15
			i = x Shr 2 : fx# = (x And 3) / 4.0
			h = gen_h(x + 1, z + 1)
			bis = h
			tief = h
			For dz = 0 To 2
				For dx = 0 To 2
					If gen_h(x + dx, z + dz) < tief Then tief = gen_h(x + dx, z + dz)
				Next
			Next
			If tief <= MEER + 1 And tief - 5 < bis Then bis = tief - 5

			; Gewoelbe: wie stark die Schwelle der Kaesehoehlen hier sinkt
			a# = gen_gew(i, k) + (gen_gew(i + 1, k) - gen_gew(i, k)) * fx
			b# = gen_gew(i, k + 1) + (gen_gew(i + 1, k + 1) - gen_gew(i, k + 1)) * fx
			gewoelbe# = Klemmen((a + (b - a) * fz - 0.02) * 1.6, 0, 0.3)

			; In der Ebene einmal je Saeule interpolieren, in der Hoehe je Block
			For j = 0 To (bis Shr 2) + 1
				gen_s1(j) = Hl_Ebene(1, i, k, j, fx, fz)
				gen_s2(j) = Hl_Ebene(2, i, k, j, fx, fz)
				gen_s3(j) = Hl_Ebene(3, i, k, j, fx, fz)
				gen_s4(j) = Hl_Ebene(4, i, k, j, fx, fz)
				gen_s5(j) = Hl_Ebene(5, i, k, j, fx, fz)
				gen_s6(j) = Hl_Ebene(6, i, k, j, fx, fz)
			Next

			For y = 5 To bis
				j = y Shr 2 : fy# = (y And 3) / 4.0
				frei = False
				unter = h - y           ; Bloecke unter der Oberflaeche

				; Kaese: Schwelle oben und ganz unten hoeher
				schwelle# = 0.34 - gewoelbe
				If unter < 18 Then schwelle = schwelle + (18 - unter) * 0.035
				If y < 22 Then schwelle = schwelle + (22 - y) * 0.03
				If schwelle < 0.5
					n# = gen_s1(j) + (gen_s1(j + 1) - gen_s1(j)) * fy
					frei = (n > schwelle)
				EndIf

				; Spaghetti: Querschnitt aus dem Dicke-Rauschen
				If Not frei
					n1# = gen_s2(j) + (gen_s2(j + 1) - gen_s2(j)) * fy
					r# = Klemmen(0.06 + (gen_s4(j) + (gen_s4(j + 1) - gen_s4(j)) * fy) * 0.16, 0.032, 0.11)
					If Abs(n1) < r
						n2# = gen_s3(j) + (gen_s3(j + 1) - gen_s3(j)) * fy
						frei = (n1 * n1 + n2 * n2 < r * r)
					EndIf
				EndIf

				; Nudeln: nicht bis an die Oberflaeche
				If (Not frei) And unter > 4
					n1 = gen_s5(j) + (gen_s5(j + 1) - gen_s5(j)) * fy
					If Abs(n1) < 0.028
						n2 = gen_s6(j) + (gen_s6(j + 1) - gen_s6(j)) * fy
						frei = (n1 * n1 + n2 * n2 < 0.028 * 0.028)
					EndIf
				EndIf

				If frei
					o = Ofs(x, y, z)
					If PeekByte(bank, o) <> B_WASSER Then PokeByte bank, o, B_LUFT
				EndIf
			Next
		Next
	Next
End Function

; Ein Rauschfeld der Hoehlen in der Ebene interpoliert (Gitterzelle i, k,
; Hoehe j, Anteil fx, fz).
Function Hl_Ebene#(feld, i, k, j, fx#, fz#)
	Select feld
	Case 1 : a00# = gen_c1(i, k, j) : a10# = gen_c1(i + 1, k, j) : a01# = gen_c1(i, k + 1, j) : a11# = gen_c1(i + 1, k + 1, j)
	Case 2 : a00 = gen_c2(i, k, j) : a10 = gen_c2(i + 1, k, j) : a01 = gen_c2(i, k + 1, j) : a11 = gen_c2(i + 1, k + 1, j)
	Case 3 : a00 = gen_c3(i, k, j) : a10 = gen_c3(i + 1, k, j) : a01 = gen_c3(i, k + 1, j) : a11 = gen_c3(i + 1, k + 1, j)
	Case 4 : a00 = gen_c4(i, k, j) : a10 = gen_c4(i + 1, k, j) : a01 = gen_c4(i, k + 1, j) : a11 = gen_c4(i + 1, k + 1, j)
	Case 5 : a00 = gen_c5(i, k, j) : a10 = gen_c5(i + 1, k, j) : a01 = gen_c5(i, k + 1, j) : a11 = gen_c5(i + 1, k + 1, j)
	Case 6 : a00 = gen_c6(i, k, j) : a10 = gen_c6(i + 1, k, j) : a01 = gen_c6(i, k + 1, j) : a11 = gen_c6(i + 1, k + 1, j)
	End Select
	a# = a00 + (a10 - a00) * fx
	b# = a01 + (a11 - a01) * fx
	Return a + (b - a) * fz
End Function

; Kohle bis ins Gebirge, Eisen bis knapp ueber den Meeresspiegel - so dicht
; wie vor der Verdopplung der Welthoehe, nur ueber mehr Gestein verteilt.
Function Erze_Setzen(bank)
	For n = 1 To 32
		Erz_Klumpen(bank, B_KOHLE, Rand(5, MEER + 42), Rand(4, 9))
	Next
	For n = 1 To 22
		Erz_Klumpen(bank, B_EISEN, Rand(5, MEER + 2), Rand(3, 6))
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

; Gras, Blumen und Schilf. Hohes Gras waechst in Flecken auf den Wiesen,
; Blumen in kleinen Beeten aus einer Art (dazu ab und zu eine einzelne).
; Schilf steht wie Zuckerrohr in Minecraft auf Gras, Erde oder Sand direkt
; neben Wasser, ein bis drei Bloecke hoch. Die Saeulen am Rand des Chunks
; bleiben dafuer frei, ihre Nachbarn liegen in einem anderen Chunk.
Function Pflanzen_Saeen(bank, x0, z0, hmax)
	For z = 0 To 15
		For x = 0 To 15
			; die Oberflaeche suchen: Baeume stehen hoeher, Tuempel tiefer
			y = gen_h(x + 1, z + 1) + 1
			If y < HOEHE - 3
				If PeekByte(bank, Ofs(x, y, z)) = B_LUFT
					While y > 1 And PeekByte(bank, Ofs(x, y - 1, z)) = B_LUFT
						y = y - 1
					Wend
					boden = PeekByte(bank, Ofs(x, y - 1, z))
					wx = x0 + x : wz = z0 + z
					If (boden = B_GRAS Or boden = B_ERDE Or boden = B_SAND) And x > 0 And x < 15 And z > 0 And z < 15
						u = y - 1
						If PeekByte(bank, Ofs(x + 1, u, z)) = B_WASSER Or PeekByte(bank, Ofs(x - 1, u, z)) = B_WASSER Or PeekByte(bank, Ofs(x, u, z + 1)) = B_WASSER Or PeekByte(bank, Ofs(x, u, z - 1)) = B_WASSER
							If Rnd(0, 1) < 0.3
								hoch = Rand(1, 3)
								For k = 0 To hoch - 1
									PokeByte bank, Ofs(x, y + k, z), B_SCHILF
								Next
								If y + hoch > hmax Then hmax = y + hoch
								boden = B_LUFT        ; hier waechst sonst nichts mehr
							EndIf
						EndIf
					EndIf
					If boden = B_GRAS
						p = B_LUFT
						beet# = Fbm2(wx / 50.0, wz / 50.0, 2, 410)
						If beet > 0.28 And Rnd(0, 1) < 0.18
							art# = Rauschen2(wx / 90.0, wz / 90.0, 470)
							If art < -0.3
								p = B_MOHN
							ElseIf art > 0.3
								p = B_KORNBLUME
							Else
								p = B_LOEWENZAHN
							EndIf
						ElseIf Rnd(0, 1) < 0.004
							p = B_LOEWENZAHN + Rand(0, 2)
						Else
							wiese# = Fbm2(wx / 36.0, wz / 36.0, 2, 330)
							If Rnd(0, 1) < Klemmen(0.14 + wiese * 0.6, 0.02, 0.5) Then p = B_HOHESGRAS
						EndIf
						If p <> B_LUFT
							PokeByte bank, Ofs(x, y, z), p
							If y > hmax Then hmax = y
						EndIf
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
