; BLTZCRFT - Inventar
;
; 36 Plaetze: 0-8 sind die Schnellleiste unten im Bild, 9-35 der Vorrat, den
; man mit E oeffnet. Ein Platz haelt einen Stapel einer Blockart, hoechstens
; INV_STAPEL Stueck. Abgebaute Bloecke wandern hinein, gesetzt wird aus dem
; gewaehlten Platz der Schnellleiste.
;
; Im offenen Inventar wie gewohnt: Linksklick nimmt einen Stapel auf, legt
; ihn ab oder tauscht ihn; Rechtsklick nimmt den halben Stapel bzw. legt ein
; Stueck ab; Umschalt+Linksklick schiebt einen Stapel zwischen Leiste und
; Vorrat hin und her.
;
; Die Symbole malt das Programm zur Laufzeit selbst: ein Wuerfel schraeg von
; oben, die drei Seiten aus den Blocktexturen gelesen (ReadPixelFast auf
; TextureBuffer) und in ein Bild geschrieben (WritePixelFast auf ImageBuffer).

Const INV_PLAETZE = 36
Const INV_LEISTE = 9
Const INV_STAPEL = 64
Const SYMBOL_GROESSE = 40   ; Kantenlaenge eines Symbols in Bildpunkten
Const INV_FELD = 52         ; Abstand der Plaetze im Bild

Dim inv_block(INV_PLAETZE - 1)
Dim inv_anzahl(INV_PLAETZE - 1)
Dim inv_symbol(B_ANZAHL - 1)

Global inv_gewaehlt             ; Platz der Schnellleiste, aus dem gesetzt wird
Global inv_offen
Global inv_hand_block, inv_hand_anzahl  ; der Stapel am Mauszeiger

; ---- Inhalt --------------------------------------------------------------

Function Inventar_Leeren()
	For i = 0 To INV_PLAETZE - 1
		inv_block(i) = B_LUFT : inv_anzahl(i) = 0
	Next
	inv_hand_block = B_LUFT : inv_hand_anzahl = 0
	inv_gewaehlt = 0
End Function

; Was ein abgebauter Block hinterlaesst. Gras verliert seine Grasnarbe.
Function Block_Ertrag(b)
	Select b
	Case B_GRAS : Return B_ERDE
	Case B_LUFT, B_WASSER, B_GRUND : Return B_LUFT
	End Select
	Return b
End Function

; Legt n Bloecke ab: erst auf angefangene Stapel, dann auf freie Plaetze -
; jeweils die Schnellleiste zuerst. Gibt zurueck, was nicht mehr passt.
Function Inventar_Hinzu(b, n)
	If b = B_LUFT Then Return 0
	For i = 0 To INV_PLAETZE - 1
		If n = 0 Then Return 0
		If inv_block(i) = b And inv_anzahl(i) < INV_STAPEL
			k = INV_STAPEL - inv_anzahl(i)
			If k > n Then k = n
			inv_anzahl(i) = inv_anzahl(i) + k : n = n - k
		EndIf
	Next
	For i = 0 To INV_PLAETZE - 1
		If n = 0 Then Return 0
		If inv_anzahl(i) = 0
			k = n
			If k > INV_STAPEL Then k = INV_STAPEL
			inv_block(i) = b : inv_anzahl(i) = k : n = n - k
		EndIf
	Next
	Return n
End Function

; Nimmt ein Stueck vom Platz; gibt die Blockart zurueck (Luft, wenn leer).
Function Inventar_Nehmen(i)
	If inv_anzahl(i) = 0 Then Return B_LUFT
	b = inv_block(i)
	inv_anzahl(i) = inv_anzahl(i) - 1
	If inv_anzahl(i) = 0 Then inv_block(i) = B_LUFT
	Return b
End Function

Function Inventar_Gewaehlt()
	If inv_anzahl(inv_gewaehlt) = 0 Then Return B_LUFT
	Return inv_block(inv_gewaehlt)
End Function

; Linksklick auf einen Platz im offenen Inventar.
Function Inventar_Links(i)
	If inv_hand_anzahl = 0
		inv_hand_block = inv_block(i) : inv_hand_anzahl = inv_anzahl(i)
		inv_block(i) = B_LUFT : inv_anzahl(i) = 0
	ElseIf inv_anzahl(i) > 0 And inv_block(i) = inv_hand_block
		k = INV_STAPEL - inv_anzahl(i)
		If k > inv_hand_anzahl Then k = inv_hand_anzahl
		inv_anzahl(i) = inv_anzahl(i) + k
		inv_hand_anzahl = inv_hand_anzahl - k
	Else
		b = inv_block(i) : n = inv_anzahl(i)
		inv_block(i) = inv_hand_block : inv_anzahl(i) = inv_hand_anzahl
		inv_hand_block = b : inv_hand_anzahl = n
	EndIf
	If inv_hand_anzahl = 0 Then inv_hand_block = B_LUFT
End Function

; Rechtsklick: mit leerer Hand den halben Stapel nehmen (aufgerundet),
; sonst ein Stueck ablegen.
Function Inventar_Rechts(i)
	If inv_hand_anzahl = 0
		If inv_anzahl(i) = 0 Then Return
		k = (inv_anzahl(i) + 1) / 2
		inv_hand_block = inv_block(i) : inv_hand_anzahl = k
		inv_anzahl(i) = inv_anzahl(i) - k
		If inv_anzahl(i) = 0 Then inv_block(i) = B_LUFT
	ElseIf inv_anzahl(i) = 0 Or (inv_block(i) = inv_hand_block And inv_anzahl(i) < INV_STAPEL)
		inv_block(i) = inv_hand_block
		inv_anzahl(i) = inv_anzahl(i) + 1
		inv_hand_anzahl = inv_hand_anzahl - 1
		If inv_hand_anzahl = 0 Then inv_hand_block = B_LUFT
	EndIf
End Function

; Umschalt+Klick: der Stapel wechselt zwischen Schnellleiste und Vorrat.
Function Inventar_Schieben(i)
	If inv_anzahl(i) = 0 Then Return
	If i < INV_LEISTE Then von = INV_LEISTE : bis = INV_PLAETZE - 1 Else von = 0 : bis = INV_LEISTE - 1
	b = inv_block(i) : n = inv_anzahl(i)
	For j = von To bis
		If inv_block(j) = b And inv_anzahl(j) < INV_STAPEL
			k = INV_STAPEL - inv_anzahl(j)
			If k > n Then k = n
			inv_anzahl(j) = inv_anzahl(j) + k : n = n - k
		EndIf
	Next
	For j = von To bis
		If n > 0 And inv_anzahl(j) = 0
			inv_block(j) = b : inv_anzahl(j) = n : n = 0
		EndIf
	Next
	inv_anzahl(i) = n
	If n = 0 Then inv_block(i) = B_LUFT
End Function

; Beim Schliessen kommt der Stapel aus der Hand zurueck ins Inventar.
Function Inventar_Schliessen()
	If inv_hand_anzahl > 0 Then Inventar_Hinzu(inv_hand_block, inv_hand_anzahl)
	inv_hand_block = B_LUFT : inv_hand_anzahl = 0
	inv_offen = False
End Function

; ---- Symbole -------------------------------------------------------------

; Farbe eines Texels der Blocktextur, abgedunkelt um hell/256.
Function Symbol_Texel(t, u#, v#, hell)
	tx = Int(u * 16) : ty = Int(v * 16)
	If tx > 15 Then tx = 15
	If ty > 15 Then ty = 15
	c = ReadPixelFast(tx * TEX_ZOOM + TEX_ZOOM / 2, ty * TEX_ZOOM + TEX_ZOOM / 2, TextureBuffer(tx_bild(t)))
	r = ((c Shr 16) And 255) * hell / 256
	g = ((c Shr 8) And 255) * hell / 256
	b = (c And 255) * hell / 256
	; reines Schwarz waere im Bild durchsichtig (Maskenfarbe)
	If r + g + b = 0 Then b = 1
	Return $FF000000 Or (r Shl 16) Or (g Shl 8) Or b
End Function

; Ein Wuerfel schraeg von oben: Deckel als Raute, darunter links und rechts
; die Seiten, unterschiedlich hell wie im Sonnenlicht. Fuer jeden Bildpunkt
; wird zurueckgerechnet, auf welcher Seite und welchem Texel er liegt.
Function Symbol_Malen(b)
	n = SYMBOL_GROESSE
	h# = n / 2.0 : q# = n / 4.0
	img = CreateImage(n, n)
	puffer = ImageBuffer(img)
	t_oben = bl_tex(b, 0) : t_seite = bl_tex(b, 1)
	LockBuffer TextureBuffer(tx_bild(t_oben))
	If t_seite <> t_oben Then LockBuffer TextureBuffer(tx_bild(t_seite))
	LockBuffer puffer
	For py = 0 To n - 1
		For px = 0 To n - 1
			x# = px + 0.5 : y# = py + 0.5
			farbe = 0
			; Deckel: Ecke oben in der Mitte, Achsen nach rechts und links unten
			dx# = (x - h) / h : dy# = y / q
			u# = (dx + dy) / 2 : v# = (dy - dx) / 2
			If u >= 0 And u < 1 And v >= 0 And v < 1
				farbe = Symbol_Texel(t_oben, u, v, 256)
			ElseIf x < h
				; linke Seite: von (0, q) nach rechts unten, senkrecht h hoch
				u = x / h : v = (y - q - u * q) / h
				If v >= 0 And v < 1 Then farbe = Symbol_Texel(t_seite, u, v, 205)
			Else
				; rechte Seite: von (h, h) nach rechts oben
				u = (x - h) / h : v = (y - h + u * q) / h
				If v >= 0 And v < 1 Then farbe = Symbol_Texel(t_seite, u, v, 150)
			EndIf
			WritePixelFast px, py, farbe, puffer
		Next
	Next
	UnlockBuffer puffer
	UnlockBuffer TextureBuffer(tx_bild(t_oben))
	If t_seite <> t_oben Then UnlockBuffer TextureBuffer(tx_bild(t_seite))
	Return img
End Function

; Nach Bloecke_Laden: ein Symbol je Blockart, die man halten kann.
Function Inventar_Symbole()
	For b = 1 To B_ANZAHL - 1
		If Block_Ertrag(b) = b Then inv_symbol(b) = Symbol_Malen(b)
	Next
End Function

; ---- Zeichnen ------------------------------------------------------------

Function Platz_Zeichnen(i, x, y, rand)
	Color 0, 0, 0 : Rect x, y, INV_FELD - 4, INV_FELD - 4, 1
	Color 90, 90, 90 : Rect x + 2, y + 2, INV_FELD - 8, INV_FELD - 8, 1
	If rand Then Color 255, 255, 255 : Rect x - 2, y - 2, INV_FELD, INV_FELD, 0 : Rect x - 1, y - 1, INV_FELD - 2, INV_FELD - 2, 0
	If inv_anzahl(i) > 0 Then Stapel_Zeichnen(inv_block(i), inv_anzahl(i), x + 4, y + 4)
End Function

Function Stapel_Zeichnen(b, n, x, y)
	If inv_symbol(b) Then DrawImage inv_symbol(b), x, y
	If n > 1
		Color 0, 0, 0 : Text_Rechts(x + SYMBOL_GROESSE + 3, y + SYMBOL_GROESSE - 11, n)
		Color 255, 255, 255 : Text_Rechts(x + SYMBOL_GROESSE + 2, y + SYMBOL_GROESSE - 12, n)
	EndIf
End Function

Function Text_Rechts(x, y, s$)
	Text x - StringWidth(s), y, s
End Function

; Linke obere Ecke von Platz i im offenen Inventar (Vorrat oben in drei
; Reihen, die Schnellleiste mit etwas Abstand darunter).
Function Platz_X(i)
	Return (GraphicsWidth() - INV_LEISTE * INV_FELD) / 2 + (i Mod INV_LEISTE) * INV_FELD + 2
End Function
Function Platz_Y(i)
	oben = GraphicsHeight() / 2 - 2 * INV_FELD
	If i < INV_LEISTE Then Return oben + 3 * INV_FELD + 16
	Return oben + ((i - INV_LEISTE) / INV_LEISTE) * INV_FELD
End Function

; Welcher Platz liegt unter dem Mauszeiger? -1 = keiner.
Function Platz_Unter(mx, my)
	For i = 0 To INV_PLAETZE - 1
		x = Platz_X(i) : y = Platz_Y(i)
		If mx >= x And mx < x + INV_FELD - 4 And my >= y And my < y + INV_FELD - 4 Then Return i
	Next
	Return -1
End Function

Function Leiste_Zeichnen()
	x0 = (GraphicsWidth() - INV_LEISTE * INV_FELD) / 2
	y = GraphicsHeight() - INV_FELD - 8
	For i = 0 To INV_LEISTE - 1
		Platz_Zeichnen(i, x0 + i * INV_FELD + 2, y, i = inv_gewaehlt)
	Next
	b = Inventar_Gewaehlt()
	If b <> B_LUFT
		Color 255, 255, 255
		Text GraphicsWidth() / 2, y - 12, bl_name(b), True, True
	EndIf
End Function

Function Inventar_Zeichnen()
	links = Platz_X(0) - 14 : oben = Platz_Y(INV_LEISTE) - 40
	breite = INV_LEISTE * INV_FELD + 24 : tief = 4 * INV_FELD + 16 + 56
	Color 40, 40, 40 : Rect links, oben, breite, tief, 1
	Color 160, 160, 160 : Rect links, oben, breite, tief, 0
	Color 255, 255, 255
	Text links + 14, oben + 12, "Inventar"
	Text_Rechts(links + breite - 14, oben + 12, "E schliesst")
	mx = MouseX() : my = MouseY()
	ueber = Platz_Unter(mx, my)
	For i = 0 To INV_PLAETZE - 1
		Platz_Zeichnen(i, Platz_X(i), Platz_Y(i), i = ueber)
	Next
	If inv_hand_anzahl > 0
		Stapel_Zeichnen(inv_hand_block, inv_hand_anzahl, mx - SYMBOL_GROESSE / 2, my - SYMBOL_GROESSE / 2)
	ElseIf ueber >= 0
		If inv_anzahl(ueber) > 0
			Color 0, 0, 0 : Rect mx + 12, my - 22, StringWidth(bl_name(inv_block(ueber))) + 8, 18, 1
			Color 255, 255, 255 : Text mx + 16, my - 20, bl_name(inv_block(ueber))
		EndIf
	EndIf
End Function
