; BLTZCRFT - Nachladen
;
; In jedem Bild bekommt die Welt ein paar Millisekunden: darin wird immer der
; naechstgelegene Chunk erzeugt (bis SICHT + 1, die Nachbarn fuer den Rand)
; oder gebaut (bis SICHT), je nachdem, was naeher liegt. Chunks weiter als
; SICHT + 3 werden vergessen - der Abstand verhindert, dass ein Chunk an der
; Grenze bei jedem Schritt hin und her geladen wird.

Global ld_offen = 0         ; Chunks in Reichweite, die noch fehlen oder alt sind
Global ld_logo = 0          ; Logo fuers Ladebild (-1: nicht gefunden)

Function Laden_Umgebung(wx, wz, r)
	cx = wx Sar 4 : cz = wz Sar 4
	For dz = -r - 1 To r + 1
		For dx = -r - 1 To r + 1
			If Chunk_Holen(cx + dx, cz + dz) = Null Then Chunk_Neu(cx + dx, cz + dz)
		Next
		Laden_Bild("Welt wird erzeugt", (dz + r + 1) / Float(2 * r + 3))
	Next
	For dz = -r To r
		For dx = -r To r
			Chunk_Bauen(Chunk_Holen(cx + dx, cz + dz))
		Next
		Laden_Bild("Welt wird gebaut", (dz + r) / Float(2 * r + 1))
	Next
End Function

; Ladebild: das Logo auf Himmelblau, darunter Text und Balken. Fehlt
; daten/logo.png, bleibt es bei Text und Balken.
Function Laden_Bild(was$, anteil#)
	If ld_logo = 0 Then ld_logo = LoadImage("daten/logo.png") : If ld_logo = 0 Then ld_logo = -1
	ClsColor 150, 196, 255
	Cls
	ClsColor 0, 0, 0
	mx = GraphicsWidth() / 2
	y = GraphicsHeight() / 2
	If ld_logo > 0
		DrawImage ld_logo, mx - ImageWidth(ld_logo) / 2, 20
		y = 20 + ImageHeight(ld_logo) + 40
	EndIf
	Color 30, 40, 70
	; anteil 1 und mehr: fertig, nur noch der Text
	If anteil >= 1
		Text mx, y, was, True, True
	Else
		Text mx, y - 20, was + " ...", True, True
		Color 60, 80, 120
		Rect mx - 150, y, 300, 12
		Color 120, 200, 90
		Rect mx - 150, y, Int(300 * anteil), 12
	EndIf
	Flip
End Function

Function Laden_Takt(pcx, pcz, budget)
	start = MilliSecs()
	Repeat
		; naechster fehlender und naechster zu bauender Chunk
		fehlt_d = 999999 : bau_d = 999999 : bau.Chunk = Null
		offen = 0
		For dz = -SICHT - 1 To SICHT + 1
			For dx = -SICHT - 1 To SICHT + 1
				d = dx * dx + dz * dz
				c.Chunk = Chunk_Holen(pcx + dx, pcz + dz)
				If c = Null
					offen = offen + 1
					If d < fehlt_d Then fehlt_d = d : fx = pcx + dx : fz = pcz + dz
				ElseIf c\veraltet And Abs(dx) <= SICHT And Abs(dz) <= SICHT
					offen = offen + 1
					If d < bau_d
						If Chunk_Baubar(c) Then bau_d = d : bau = c
					EndIf
				EndIf
			Next
		Next
		ld_offen = offen

		If bau <> Null And bau_d <= fehlt_d
			Chunk_Bauen(bau)
		ElseIf fehlt_d < 999999
			Chunk_Neu(fx, fz)
		Else
			Exit
		EndIf
	Until MilliSecs() - start >= budget

	; zu weit weg: vergessen
	c.Chunk = First Chunk
	While c <> Null
		n.Chunk = After c
		If Abs(c\cx - pcx) > SICHT + 3 Or Abs(c\cz - pcz) > SICHT + 3 Then Chunk_Weg(c)
		c = n
	Wend
End Function
