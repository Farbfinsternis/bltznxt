; Leuchtturm - Zielscheiben (Schritt 4).
;
; An jeder Marke "ziel" der Karte schwebt eine Scheibe (daten/ziel.glb),
; dreht sich und wippt. Sie haelt ZL_LEBEN Punkte aus, blitzt bei jedem
; Treffer auf, zerplatzt bei null und kommt nach ZL_WIEDER Sekunden zurueck.
;
; Die Scheiben sind fuer LinePick da (EntityPickMode 2, genau auf die
; Dreiecke) und fuer die Kollision der Raketen (EntityType TYP_ZIEL).
; Eine zerplatzte Scheibe ist versteckt und nicht mehr pickbar.
;
;   Ziele_Laden datei$
;   Ziele_Takt                          ; je Takt, vor UpdateWorld
;   Ziel_Treffer z, schaden             ; z aus Ziel_Von(entity)

Const ZL_LEBEN = 100
Const ZL_WIEDER# = 5.0

Global zl_vorlage
Global zl_treffer, zl_zerstoert     ; Zaehler fuer die Anzeige

Type Ziel
	Field ent
	Field x#, y#, z#
	Field leben
	Field blitz#            ; > 0: leuchtet noch so lange
	Field weg#              ; > 0: zerplatzt, kommt in so vielen Sekunden wieder
	Field phase#
End Type

Function Ziele_Laden(datei$)
	zl_vorlage = LoadMesh(datei)
	If zl_vorlage = 0 Then RuntimeError "Zielscheibe nicht lesbar: " + datei
	HideEntity zl_vorlage
	Delete Each Ziel
	n = 0
	For m.Marke = Each Marke
		If m\name = "ziel"
			z.Ziel = New Ziel
			z\ent = CopyEntity(zl_vorlage)
			ShowEntity z\ent
			z\x = m\x : z\y = m\y : z\z = m\z
			z\phase = n * 0.7           ; nicht alle im Gleichschritt
			PositionEntity z\ent, z\x, z\y, z\z
			EntityType z\ent, TYP_ZIEL
			Ziel_Neu(z)
			n = n + 1
		EndIf
	Next
	zl_treffer = 0 : zl_zerstoert = 0
End Function

Function Ziel_Neu(z.Ziel)
	z\leben = ZL_LEBEN
	z\blitz = 0 : z\weg = 0
	ShowEntity z\ent
	EntityPickMode z\ent, 2
	ScaleEntity z\ent, 1, 1, 1
	EntityFX z\ent, 2
End Function

Function Ziel_Von.Ziel(e)
	If e = 0 Then Return Null
	For z.Ziel = Each Ziel
		If z\ent = e Then Return z
	Next
	Return Null
End Function

Function Ziel_Treffer(z.Ziel, schaden)
	If z\weg > 0 Then Return
	zl_treffer = zl_treffer + 1
	z\leben = z\leben - schaden
	z\blitz = 0.08
	If z\leben > 0 Then Klang(kl_treffer, z\ent) : Return

	; Zerplatzen: rote und weisse Splitter in alle Richtungen
	zl_zerstoert = zl_zerstoert + 1
	z\weg = ZL_WIEDER
	HideEntity z\ent
	EntityPickMode z\ent, 0
	x# = EntityX(z\ent) : y# = EntityY(z\ent) : zz# = EntityZ(z\ent)
	Effekt_Klang(x, y, zz, kl_zerplatzen, 0.8)
	Effekt_Sprite(x, y, zz, fx_glanz, 3, 255, 200, 160, 0.6, 2.2, 1, 0, 0.35)
	For i = 1 To 14
		If i Mod 2 Then r = 255 : g = 60 : b = 40 Else r = 255 : g = 255 : b = 255
		Effekt_Sprite(x, y, zz, fx_glanz, 3, r, g, b, 0.25, 0.05, 1, 0.2, Rnd(0.4, 0.8), Rnd(-4, 4), Rnd(-1, 5), Rnd(-4, 4))
	Next
End Function

Function Ziele_Takt()
	For z.Ziel = Each Ziel
		z\phase = z\phase + TAKT
		If z\weg > 0
			z\weg = z\weg - TAKT
			If z\weg <= 0
				Ziel_Neu(z)
				Klang(kl_wieder, z\ent)
				Effekt_Sprite(z\x, z\y, z\z, fx_glanz, 3, 160, 220, 255, 1.6, 0.4, 0, 1, 0.3)
			EndIf
		Else
			PositionEntity z\ent, z\x, z\y + 0.15 * Sin(z\phase * 120), z\z
			RotateEntity z\ent, 0, z\phase * 60, 0
			If z\blitz > 0
				z\blitz = z\blitz - TAKT
				; Aufblitzen: voll hell und etwas groesser
				EntityFX z\ent, 1 + 2
				ScaleEntity z\ent, 1.12, 1.12, 1.12
				If z\blitz <= 0
					EntityFX z\ent, 2
					ScaleEntity z\ent, 1, 1, 1
				EndIf
			EndIf
		EndIf
	Next
End Function
