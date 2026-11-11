; BLTZCRFT - Bruchstuecke beim Abbauen
;
; Schlaegt die Hand auf einen Block, springen von der getroffenen Flaeche
; kleine Stuecke ab; bricht er, fliegt eine ganze Wolke davon auseinander.
; Jedes Stueck ist ein kleiner Wuerfel in einer Farbe, die aus der Textur
; des Blocks stammt - so sehen sie aus wie Bruchstuecke davon. Sie fallen,
; bleiben auf festen Bloecken liegen und schrumpfen am Ende weg.
;
; Die Wuerfel liegen in einem festen Vorrat und werden reihum wiederbenutzt;
; ist er erschoepft, verschwindet das aelteste Stueck zuerst. Beleuchtet
; sind sie wie die Welt (Umgebungslicht), nachts also dunkler.

Const PT_MAX = 160
Const PT_PROBEN = 8             ; Farben je Textur
Const PT_SCHWERE# = 18.0

Dim pt_e(PT_MAX - 1)
Dim pt_x#(PT_MAX - 1), pt_y#(PT_MAX - 1), pt_z#(PT_MAX - 1)
Dim pt_vx#(PT_MAX - 1), pt_vy#(PT_MAX - 1), pt_vz#(PT_MAX - 1)
Dim pt_leben#(PT_MAX - 1), pt_gr#(PT_MAX - 1)
Dim pt_probe(T_ANZAHL - 1, PT_PROBEN - 1)
Global pt_naechstes
Global pt_takt#                 ; Zeit bis zu den naechsten Stuecken beim Schlagen

; Nach Bloecke_Laden: der Vorrat an Wuerfeln und je Textur ein paar Farben.
Function Partikel_Laden()
	For i = 0 To PT_MAX - 1
		pt_e(i) = CreateCube()
		HideEntity pt_e(i)
	Next
	SeedRnd 31
	For t = 0 To T_ANZAHL - 1
		puffer = TextureBuffer(tx_bild(t))
		LockBuffer puffer
		For k = 0 To PT_PROBEN - 1
			; nicht in ein Loch greifen (Laub hat durchsichtige Stellen)
			For versuch = 1 To 20
				c = ReadPixelFast(Rand(0, 15) * TEX_ZOOM + 3, Rand(0, 15) * TEX_ZOOM + 3, puffer)
				If (c Shr 24) And 255 Then Exit
			Next
			pt_probe(t, k) = c And $FFFFFF
		Next
		UnlockBuffer puffer
	Next
End Function

Function Partikel_Neu(x#, y#, z#, vx#, vy#, vz#, b)
	i = pt_naechstes
	pt_naechstes = (pt_naechstes + 1) Mod PT_MAX
	pt_x(i) = x : pt_y(i) = y : pt_z(i) = z
	pt_vx(i) = vx : pt_vy(i) = vy : pt_vz(i) = vz
	pt_leben(i) = Rnd(0.5, 1.1)
	pt_gr(i) = Rnd(0.022, 0.042)
	c = pt_probe(bl_tex(b, 1), Rand(0, PT_PROBEN - 1))
	l# = Licht_Bei(x, y, z)             ; in einer Hoehle dunkel
	EntityColor pt_e(i), ((c Shr 16) And 255) * l, ((c Shr 8) And 255) * l, (c And 255) * l
	ScaleEntity pt_e(i), pt_gr(i), pt_gr(i), pt_gr(i)
	PositionEntity pt_e(i), x, y, z
	ShowEntity pt_e(i)
End Function

; Jedes Bild, solange die Hand auf Block b bei x,y,z schlaegt; n ist die
; Richtung der getroffenen Flaeche (-1, 0 oder 1 je Achse).
Function Partikel_Schlag(x, y, z, nx, ny, nz, b, dt#)
	If b = B_LUFT Or b = B_WASSER Then Return
	pt_takt = pt_takt - dt
	If pt_takt > 0 Then Return
	pt_takt = 0.09
	For k = 1 To 2
		px# = x + 0.5 + nx * 0.53 : py# = y + 0.5 + ny * 0.53 : pz# = z + 0.5 + nz * 0.53
		If nx = 0 Then px = px + Rnd(-0.4, 0.4)
		If ny = 0 Then py = py + Rnd(-0.4, 0.4)
		If nz = 0 Then pz = pz + Rnd(-0.4, 0.4)
		Partikel_Neu(px, py, pz, nx * Rnd(0.8, 2) + Rnd(-0.8, 0.8), ny * Rnd(0.8, 2) + Rnd(0.8, 2.4), nz * Rnd(0.8, 2) + Rnd(-0.8, 0.8), b)
	Next
End Function

; Block b bei x,y,z ist gebrochen: Stuecke aus dem ganzen Wuerfel.
Function Partikel_Bruch(x, y, z, b)
	If b = B_LUFT Or b = B_WASSER Then Return
	For i = 0 To 2
		For j = 0 To 2
			For k = 0 To 2
				ox# = (i - 1) * 0.3 + Rnd(-0.1, 0.1)
				oy# = (j - 1) * 0.3 + Rnd(-0.1, 0.1)
				oz# = (k - 1) * 0.3 + Rnd(-0.1, 0.1)
				Partikel_Neu(x + 0.5 + ox, y + 0.5 + oy, z + 0.5 + oz, ox * Rnd(3, 6), oy * Rnd(3, 6) + Rnd(1, 3), oz * Rnd(3, 6), b)
			Next
		Next
	Next
End Function

Function Partikel_Takt(dt#)
	For i = 0 To PT_MAX - 1
		If pt_leben(i) > 0
			pt_leben(i) = pt_leben(i) - dt
			If pt_leben(i) <= 0
				HideEntity pt_e(i)
			Else
				g# = pt_gr(i)
				pt_vy(i) = pt_vy(i) - PT_SCHWERE * dt
				; Achse fuer Achse: stoesst das Stueck an, bleibt es dort stehen
				nx# = pt_x(i) + pt_vx(i) * dt
				If bl_fest(Welt_Block(Floor(nx), Floor(pt_y(i)), Floor(pt_z(i)))) Then pt_vx(i) = 0 Else pt_x(i) = nx
				nz# = pt_z(i) + pt_vz(i) * dt
				If bl_fest(Welt_Block(Floor(pt_x(i)), Floor(pt_y(i)), Floor(nz))) Then pt_vz(i) = 0 Else pt_z(i) = nz
				ny# = pt_y(i) + pt_vy(i) * dt
				If bl_fest(Welt_Block(Floor(pt_x(i)), Floor(ny - g), Floor(pt_z(i))))
					If pt_vy(i) < 0
						; liegt auf: auf der Oberkante, rutscht aus
						pt_y(i) = Floor(ny - g) + 1 + g
						pt_vx(i) = pt_vx(i) * 0.6 : pt_vz(i) = pt_vz(i) * 0.6
					EndIf
					pt_vy(i) = 0
				Else
					pt_y(i) = ny
				EndIf
				PositionEntity pt_e(i), pt_x(i), pt_y(i), pt_z(i)
				; die letzte Viertelsekunde schrumpft es weg
				If pt_leben(i) < 0.25
					f# = g * pt_leben(i) / 0.25
					ScaleEntity pt_e(i), f, f, f
				EndIf
			EndIf
		EndIf
	Next
End Function
