; Leuchtturm - Effekte (Schritt 4): Funken, Rauch, Explosionen,
; Einschlagflecken, die Spur der Railgun und das Licht einer Explosion,
; dazu Klaenge an einem festen Ort (Effekt_Klang).
;
; Alles, was nur zu sehen ist und nach einer Zeit verschwindet, ist ein
; Effekt: eine Entity, die ueber ihre Lebensdauer die Groesse (Sprites) und
; die Deckkraft von einem Anfangs- zu einem Endwert aendert und dabei
; driften kann. Danach wird sie freigegeben. Ein Licht wird statt blasser
; dunkler. Ein Ort ist ein Pivot, an dem ein Klang haengt: er verschwindet
; nach der Laenge des Klangs, der Klang bliebe sonst an seiner letzten
; Stelle ohnehin stehen.
;
; Die Texturen erzeugt das Programm selbst - drei weiche Kreise:
;
;   fx_glanz   hell, fuer additive Sprites (Funken, Feuer, Spur)
;   fx_rauch   weiss mit weichem Rand, fuer Rauch mit Alpha
;   fx_fleck   schwarz mit weichem Rand, fuer Einschlaege an der Wand
;
; Einschlagflecken bleiben FX_FLECK_DAUER Sekunden; mehr als FX_FLECK_MAX
; gleichzeitig gibt es nicht, der aelteste macht dem neuen Platz.
;
;   Effekte_Laden
;   Effekte_Takt                ; je Takt
;   Effekte_Leeren              ; alle weg

Const FX_SPRITE = 1, FX_NETZ = 2, FX_LICHT = 3, FX_ORT = 4
Const FX_FLECK_MAX = 64
Const FX_FLECK_DAUER# = 12.0

Global fx_glanz, fx_rauch, fx_fleck
Global fx_flecken

Type Effekt
	Field ent, art
	Field alter#, dauer#
	Field g0#, g1#          ; Groesse (Sprite) bzw. Reichweite (Licht)
	Field a0#, a1#          ; Deckkraft bzw. Helligkeit (Licht)
	Field r, g, b           ; Farbe des Lichts
	Field vx#, vy#, vz#
	Field fleck
End Type

Function Effekte_Laden()
	fx_glanz = Effekt_Kreis(255, 255, 255, 2.0, True)
	fx_rauch = Effekt_Kreis(255, 255, 255, 1.0, False)
	fx_fleck = Effekt_Kreis(0, 0, 0, 0.6, False)
End Function

; Weicher Kreis: Deckkraft (1 - d)^weich, vom Mittelpunkt (d = 0) zum
; Rand (d = 1). "hell": auch die Farbe laeuft zum Rand aus, fuer additive
; Mischung.
Function Effekt_Kreis(r, g, b, weich#, hell)
	t = CreateTexture(32, 32, 2 + 16 + 32)
	LockBuffer TextureBuffer(t)
	For y = 0 To 31
		For x = 0 To 31
			dx# = (x - 15.5) / 15.5 : dy# = (y - 15.5) / 15.5
			d# = Sqr(dx * dx + dy * dy)
			a# = 0
			If d < 1 Then a = (1 - d) ^ weich
			If hell
				k# = a
			Else
				k# = 1
			EndIf
			WritePixelFast x, y, (Int(a * 255) Shl 24) Or (Int(r * k) Shl 16) Or (Int(g * k) Shl 8) Or Int(b * k), TextureBuffer(t)
		Next
	Next
	UnlockBuffer TextureBuffer(t)
	Return t
End Function

; Ein Sprite, das in `dauer` Sekunden von Groesse g0 auf g1 und von
; Deckkraft a0 auf a1 geht und dabei mit (vx, vy, vz) driftet.
; blend: 1 Alpha, 3 additiv.
Function Effekt_Sprite.Effekt(x#, y#, z#, tex, blend, r, g, b, g0#, g1#, a0#, a1#, dauer#, vx# = 0, vy# = 0, vz# = 0)
	s = CreateSprite()
	EntityTexture s, tex
	EntityBlend s, blend
	EntityFX s, 1 + 8
	EntityColor s, r, g, b
	PositionEntity s, x, y, z
	RotateSprite s, Rnd(360)
	f.Effekt = New Effekt
	f\ent = s : f\art = FX_SPRITE
	f\dauer = dauer
	f\g0 = g0 : f\g1 = g1 : f\a0 = a0 : f\a1 = a1
	f\vx = vx : f\vy = vy : f\vz = vz
	Effekt_Stellen(f)
	Return f
End Function

; Einschlagfleck an einer Flaeche mit der Normalen (nx, ny, nz).
Function Effekt_Fleck(x#, y#, z#, nx#, ny#, nz#, groesse#, r = 0, g = 0, b = 0, a# = 0.8)
	If fx_flecken >= FX_FLECK_MAX
		For alt.Effekt = Each Effekt
			If alt\fleck Then Effekt_Weg(alt) : Exit
		Next
	EndIf
	; Knapp vor der Flaeche, sonst flimmert er mit ihr.
	f.Effekt = Effekt_Sprite(x + nx * 0.01, y + ny * 0.01, z + nz * 0.01, fx_fleck, 1, r, g, b, groesse, groesse, a, a, FX_FLECK_DAUER)
	SpriteViewMode f\ent, 2
	AlignToVector f\ent, -nx, -ny, -nz, 3
	EntityFX f\ent, 1 + 8 + 16
	f\fleck = True
	fx_flecken = fx_flecken + 1
End Function

; Die Spur der Railgun: zwei gekreuzte Baender von (x0,y0,z0) nach
; (x1,y1,z1), damit sie von jeder Seite gleich breit aussieht.
Function Effekt_Spur(x0#, y0#, z0#, x1#, y1#, z1#, r, g, b, breite#, dauer#)
	dx# = x1 - x0 : dy# = y1 - y0 : dz# = z1 - z0
	l# = Sqr(dx * dx + dy * dy + dz * dz)
	If l < 0.01 Then Return
	dx = dx / l : dy = dy / l : dz = dz / l
	; u senkrecht zur Spur und zur Hochachse (oder zur x-Achse, wenn die
	; Spur senkrecht verlaeuft), v senkrecht zu beiden.
	If Abs(dy) < 0.9
		ux# = dz : uy# = 0 : uz# = -dx
	Else
		ux# = 0 : uy# = -dz : uz# = dy
	EndIf
	ul# = Sqr(ux * ux + uy * uy + uz * uz)
	ux = ux / ul * breite / 2 : uy = uy / ul * breite / 2 : uz = uz / ul * breite / 2
	vx# = dy * uz - dz * uy : vy# = dz * ux - dx * uz : vz# = dx * uy - dy * ux

	m = CreateMesh()
	s = CreateSurface(m)
	Effekt_Band(s, x0, y0, z0, x1, y1, z1, ux, uy, uz)
	Effekt_Band(s, x0, y0, z0, x1, y1, z1, vx, vy, vz)
	EntityTexture m, fx_glanz
	EntityBlend m, 3
	EntityFX m, 1 + 8 + 16
	EntityColor m, r, g, b
	f.Effekt = New Effekt
	f\ent = m : f\art = FX_NETZ
	f\dauer = dauer
	f\a0 = 1 : f\a1 = 0
	Effekt_Stellen(f)
End Function

Function Effekt_Band(s, x0#, y0#, z0#, x1#, y1#, z1#, ux#, uy#, uz#)
	; Quer zur Spur laeuft die Textur einmal hinueber (u), laengs bleibt sie
	; in ihrer Mitte (v = 0.5): hell in der Mitte, weich zum Rand.
	a = AddVertex(s, x0 - ux, y0 - uy, z0 - uz, 0, 0.5)
	b = AddVertex(s, x0 + ux, y0 + uy, z0 + uz, 1, 0.5)
	c = AddVertex(s, x1 + ux, y1 + uy, z1 + uz, 1, 0.5)
	d = AddVertex(s, x1 - ux, y1 - uy, z1 - uz, 0, 0.5)
	AddTriangle s, a, b, c
	AddTriangle s, a, c, d
End Function

; Punktlicht, das in `dauer` Sekunden verlischt.
Function Effekt_Licht(x#, y#, z#, r, g, b, reichweite#, dauer#)
	l = CreateLight(2)
	PositionEntity l, x, y, z
	f.Effekt = New Effekt
	f\ent = l : f\art = FX_LICHT
	f\r = r : f\g = g : f\b = b
	f\dauer = dauer
	f\g0 = reichweite : f\g1 = reichweite
	f\a0 = 1 : f\a1 = 0
	Effekt_Stellen(f)
End Function

; Klang an einem Ort, fuer `dauer` Sekunden (klang.bb).
Function Effekt_Klang(x#, y#, z#, snd, dauer#)
	If snd = 0 Then Return
	o = CreatePivot()
	PositionEntity o, x, y, z
	f.Effekt = New Effekt
	f\ent = o : f\art = FX_ORT
	f\dauer = dauer
	Klang(snd, o)
End Function

Function Effekt_Stellen(f.Effekt)
	t# = f\alter / f\dauer
	If t > 1 Then t = 1
	a# = f\a0 + (f\a1 - f\a0) * t
	g# = f\g0 + (f\g1 - f\g0) * t
	Select f\art
	Case FX_SPRITE
		ScaleSprite f\ent, g / 2, g / 2
		If f\fleck
			; Flecken verblassen erst im letzten Viertel.
			t = (t - 0.75) * 4
			If t < 0 Then t = 0
			EntityAlpha f\ent, f\a0 * (1 - t)
		Else
			EntityAlpha f\ent, a
		EndIf
	Case FX_NETZ
		EntityAlpha f\ent, a
	Case FX_LICHT
		LightColor f\ent, f\r * a, f\g * a, f\b * a
		LightRange f\ent, g
	End Select
End Function

Function Effekt_Weg(f.Effekt)
	If f\fleck Then fx_flecken = fx_flecken - 1
	FreeEntity f\ent
	Delete f
End Function

Function Effekte_Takt()
	For f.Effekt = Each Effekt
		f\alter = f\alter + TAKT
		If f\alter >= f\dauer
			Effekt_Weg(f)
		Else
			If f\vx <> 0 Or f\vy <> 0 Or f\vz <> 0 Then TranslateEntity f\ent, f\vx * TAKT, f\vy * TAKT, f\vz * TAKT
			Effekt_Stellen(f)
		EndIf
	Next
End Function

Function Effekte_Leeren()
	For f.Effekt = Each Effekt
		Effekt_Weg(f)
	Next
End Function
