; Friendly Fire - die Anzeige (Schritt 7).
;
; Unten links die Munition der Waffe in der Hand, in der Mitte das Leben,
; rechts die Ruestung - gross, wie in Quake III; darueber klein die Waffen,
; die man hat. Was man aufsammelt, steht zwei Sekunden in der Bildmitte.
; Schaden faerbt das Bild kurz rot: ein rotes Sprite dicht vor der Kamera,
; so deckend, wie der Schaden frisch ist (p\schmerz). Das Leben ist ueber
; 100 weiss, darunter gelb, unter 25 rot.
;
;   Anzeige_Laden p
;   Anzeige_Takt p              ; je Takt
;   Anzeige_Zeichnen p          ; nach RenderWorld

Global az_gross, az_klein
Global az_rot

Function Anzeige_Laden(p.Spieler)
	az_gross = LoadFont("Arial", 44, True)
	az_klein = LoadFont("Arial", 16)
	If az_klein Then SetFont az_klein
	az_rot = CreateSprite(p\kamera)
	PositionEntity az_rot, 0, 0, 0.07
	ScaleSprite az_rot, 0.2, 0.2
	EntityColor az_rot, 255, 0, 0
	EntityFX az_rot, 1 + 8
	HideEntity az_rot
End Function

Function Anzeige_Takt(p.Spieler)
	a# = p\schmerz * 0.35
	If p\tot Then a = 0.45
	If a > 0
		ShowEntity az_rot
		EntityAlpha az_rot, a
	Else
		HideEntity az_rot
	EndIf
End Function

Function Anzeige_Zeichnen(p.Spieler)
	w = GraphicsWidth() : h = GraphicsHeight()
	y = h - 64

	m = w_munition(w_aktiv)
	If m <= 5 Then r = 255 : g = 60 : b = 40 Else r = 255 : g = 230 : b = 120
	Anzeige_Wert(w * 0.15, y, m, w_name(w_aktiv), r, g, b)

	If p\leben > 100
		r = 255 : g = 255 : b = 255
	ElseIf p\leben >= 25
		r = 255 : g = 230 : b = 120
	Else
		r = 255 : g = 60 : b = 40
	EndIf
	l = p\leben : If l < 0 Then l = 0
	Anzeige_Wert(w * 0.5, y, l, "Leben", r, g, b)
	Anzeige_Wert(w * 0.85, y, p\panzer, "Ruestung", 150, 230, 150)

	; die Waffen, die man hat; die gewaehlte hell
	If az_klein Then SetFont az_klein
	t$ = ""
	For i = 1 To W_ANZAHL
		If w_besitz(i) Then t = t + "  " + i + " " + w_name(i)
	Next
	x = w * 0.15 - StringWidth(t) / 2
	If x < 8 Then x = 8
	For i = 1 To W_ANZAHL
		If w_besitz(i)
			If i = w_wunsch Then Color 255, 230, 120 Else Color 130, 130, 130
			s$ = "  " + i + " " + w_name(i)
			Text x, y - 44, s
			x = x + StringWidth(s)
		EndIf
	Next

	If it_meldung_zeit > 0
		Color 255, 255, 255
		Text w / 2, h * 0.3, it_meldung, True, True
	EndIf
	If p\tot
		If az_gross Then SetFont az_gross
		Color 255, 255, 255
		Text w / 2, h * 0.4, "Du bist gestorben", True, True
		If az_klein Then SetFont az_klein
	EndIf
End Function

Function Anzeige_Wert(x, y, zahl, titel$, r, g, b)
	If az_klein Then SetFont az_klein
	Color 170, 170, 170
	Text x, y - 20, titel, True
	If az_gross Then SetFont az_gross
	Color r, g, b
	Text x, y, zahl, True
	If az_klein Then SetFont az_klein
End Function
