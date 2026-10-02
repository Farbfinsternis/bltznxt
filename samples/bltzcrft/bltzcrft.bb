; BLTZCRFT - eine Welt aus Bloecken im Stil von Minecraft, geschrieben als
; gewoehnliches Blitz3D-Programm und uebersetzt mit BLTZNXT.
;
; Stand: erste Fassung - die Welt entsteht in Chunks aus einer Saat
; (Gelaende, Ozean, Seen, Tuempel, Hoehlen, Erze, Baeume) und wird um den
; Spieler herum nachgeladen; man laeuft in der Ego-Sicht hindurch.
; Bloecke abbauen und setzen geht auch schon, gespeichert wird nichts.
;
; Steuerung:  Klick ins Fenster faengt die Maus, Tab gibt sie frei, Esc beendet
;             Maus schauen   W A S D / Pfeile laufen   Leertaste springen,
;             schwimmen      Strg rennen     F fliegen (Leertaste hoch,
;             Umschalt runter)
;             linke Maustaste Block abbauen  rechte Maustaste Block setzen
;             1 - 9 Block waehlen            F3 Anzeige
;
; Aufruf mit einer Zahl als Saat: bltzcrft.exe 1234 - ohne Zahl 2026.
; Mit -rundflug fliegt das Programm 20 s von selbst geradeaus, misst die
; Bildzeiten, gibt sie aus (auch in rundflug.txt) und beendet sich
; (bltzcrft.exe 2026 -rundflug).
;
; Protokoll der Stellen, an denen BLTZNXT die Arbeit erschwert hat:
; PROTOKOLL.md in diesem Ordner.

Const SICHT = 8             ; Sichtweite in Chunks
Const MAUS_EMPF# = 0.15     ; Grad je Pixel
Const BAU_ZEIT = 7          ; Millisekunden je Bild fuer Erzeugen und Bauen

Include "rauschen.bb"
Include "bloecke.bb"
Include "welt.bb"
Include "netz.bb"
Include "spieler.bb"
Include "laden.bb"

AppTitle "BLTZCRFT"
Graphics3D 1280, 720, 0, 2
SetBuffer BackBuffer()

saat = 2026
args$ = Lower(Trim(CommandLine()))
rundflug = (Instr(args, "-rundflug") > 0)
If args <> "" And Left(args, 1) <> "-" Then saat = Int(args)

Bloecke_Laden()
Netz_Laden()
Welt_Saat(saat)

; Start: die naechste Stelle an Land, vom Ursprung aus spiralfoermig gesucht
sx = 0 : sz = 0
For r = 0 To 2000 Step 16
	gefunden = False
	For w = 0 To 359 Step 15
		x = Int(Cos(w) * r) : z = Int(Sin(w) * r)
		If Gelaende_Hoehe(x, z) > MEER + 2
			sx = x : sz = z : gefunden = True
			Exit
		EndIf
		If r = 0 Then Exit
	Next
	If gefunden Then Exit
Next

; Erst die Umgebung erzeugen, dann den Spieler auf den Boden stellen
Spieler_Neu(sx + 0.5, HOEHE, sz + 0.5)
Laden_Umgebung(sx, sz, 3)
sy = HOEHE - 1
While sy > 0 And (Not bl_fest(Welt_Block(sx, sy, sz)))
	sy = sy - 1
Wend
Spieler_Setzen(sx + 0.5, sy + 1, sz + 0.5)

Dim auswahl(8)
Restore auswahl_daten
For i = 0 To 8 : Read auswahl(i) : Next
gewaehlt = 0

anzeige = True
gefangen = False
sprung_merken = False

; Titelbild stehen lassen, bis man klickt - der Klick faengt die Maus.
; Esc beendet schon hier.
ende = False
If Not rundflug
	FlushKeys : FlushMouse
	Repeat
		Laden_Bild("Klick zum Spielen - Esc beendet", 1)
		If KeyHit(1) Then ende = True
		If MouseHit(1)
			gefangen = True
			HidePointer
			MoveMouse GraphicsWidth() / 2, GraphicsHeight() / 2
			MouseXSpeed() : MouseYSpeed()
		EndIf
	Until gefangen Or ende
EndIf
If ld_logo > 0 Then FreeImage ld_logo

zeit = MilliSecs()
rest# = 0
bilder = 0 : bilder_zeit = MilliSecs() : fps = 0
bau_ms = 0

; dunkler Rahmen um den anvisierten Block
rahmen = CreateCube()
ScaleEntity rahmen, 0.505, 0.505, 0.505
EntityColor rahmen, 0, 0, 0
EntityAlpha rahmen, 0.25
EntityFX rahmen, 1
HideEntity rahmen

While Not ende
	If KeyHit(61) Then anzeige = Not anzeige
	If KeyHit(1) Then ende = True
	If gefangen
		If KeyHit(15)
			gefangen = False
			ShowPointer
		EndIf
	ElseIf Not rundflug
		If MouseHit(1)
			gefangen = True
			HidePointer
			MoveMouse GraphicsWidth() / 2, GraphicsHeight() / 2
			MouseXSpeed() : MouseYSpeed()
			MouseHit(1) : MouseHit(2)
		EndIf
	EndIf

	vor# = 0 : seit# = 0 : hoch = False : runter = False : rennen = False
	If gefangen
		Spieler_Schauen(-MouseXSpeed() * MAUS_EMPF, MouseYSpeed() * MAUS_EMPF)
		MoveMouse GraphicsWidth() / 2, GraphicsHeight() / 2
		If KeyDown(17) Or KeyDown(200) Then vor = vor + 1
		If KeyDown(31) Or KeyDown(208) Then vor = vor - 1
		If KeyDown(32) Or KeyDown(205) Then seit = seit + 1
		If KeyDown(30) Or KeyDown(203) Then seit = seit - 1
		hoch = KeyDown(57)
		runter = KeyDown(42) Or KeyDown(54)
		rennen = KeyDown(29) Or KeyDown(157)
		If KeyHit(33)
			sp_fliegen = Not sp_fliegen
			sp_vy = 0
		EndIf
		For i = 0 To 8
			If KeyHit(2 + i) Then gewaehlt = i
		Next
		rad = MouseZSpeed()
		If rad < 0 Then gewaehlt = (gewaehlt + 1) Mod 9
		If rad > 0 Then gewaehlt = (gewaehlt + 8) Mod 9
	EndIf
	If rundflug
		; vorgegebener Flug: geradeaus, die Hoehe haelt sich ueber dem Gelaende
		vor = 1 : sp_fliegen = True
		boden_y = Gelaende_Hoehe(sp_x + Sin(-sp_gier) * 20, sp_z + Cos(sp_gier) * 20)
		If boden_y < MEER Then boden_y = MEER
		hoch = (sp_y < boden_y + 12) : runter = (sp_y > boden_y + 18)
		sp_neig = 12
		If rf_start = 0 Then rf_start = MilliSecs() : rf_letzt = rf_start : rf_x# = sp_x : rf_z# = sp_z
		dt = MilliSecs() - rf_letzt : rf_letzt = MilliSecs()
		If dt > 0
			rf_bilder = rf_bilder + 1 : rf_summe = rf_summe + dt
			If dt > rf_max Then rf_max = dt
			If dt > 33 Then rf_ueber33 = rf_ueber33 + 1
		EndIf
		If MilliSecs() - rf_start > 20000
			weg# = Sqr((sp_x - rf_x) * (sp_x - rf_x) + (sp_z - rf_z) * (sp_z - rf_z))
			erg$ = "Rundflug: " + rf_bilder + " Bilder in 20 s, Mittel " + Float(rf_summe) / rf_bilder + " ms, laengstes " + rf_max + " ms, ueber 33 ms: " + rf_ueber33 + ", Strecke " + Int(weg) + " m, Chunks " + welt_chunks
			Print erg
			; ohne Konsole (Doppelklick, Verknuepfung) sieht man Print nicht
			datei = WriteFile("rundflug.txt")
			If datei Then WriteLine datei, erg : CloseFile datei
			ende = True
		EndIf
	EndIf

	jetzt = MilliSecs()
	rest = rest + (jetzt - zeit) / 1000.0
	zeit = jetzt
	If rest > 0.25 Then rest = 0.25
	While rest >= TAKT
		Spieler_Takt(vor, seit, hoch, runter, rennen)
		rest = rest - TAKT
	Wend
	Spieler_Kamera(rest / TAKT)

	; Abbauen und Setzen
	If Blick_Strahl()
		ShowEntity rahmen
		PositionEntity rahmen, ziel_x + 0.5, ziel_y + 0.5, ziel_z + 0.5
		If gefangen
			If MouseHit(1)
				If Welt_Block(ziel_x, ziel_y, ziel_z) <> B_GRUND
					Welt_Setzen(ziel_x, ziel_y, ziel_z, B_LUFT)
				EndIf
			EndIf
			If MouseHit(2)
				If Platz_Frei(ziel_vx, ziel_vy, ziel_vz)
					Welt_Setzen(ziel_vx, ziel_vy, ziel_vz, auswahl(gewaehlt))
				EndIf
			EndIf
		EndIf
	Else
		HideEntity rahmen
		MouseHit(1) : MouseHit(2)
	EndIf

	; Chunks erzeugen, bauen und vergessen - so viel, wie in die Zeit passt
	t0 = MilliSecs()
	Laden_Takt(Floor(sp_x) Sar 4, Floor(sp_z) Sar 4, BAU_ZEIT)
	bau_ms = MilliSecs() - t0

	RenderWorld

	bilder = bilder + 1
	If MilliSecs() - bilder_zeit >= 1000
		fps = bilder : bilder = 0 : bilder_zeit = MilliSecs()
	EndIf
	Color 255, 255, 255
	If anzeige
		Text 10, 10, "BLTZCRFT   Saat " + saat + "   " + fps + " fps   (F3 Anzeige, Tab Maus frei)"
		Text 10, 28, "x " + Int(Floor(sp_x)) + "  y " + Int(Floor(sp_y)) + "  z " + Int(Floor(sp_z)) + "   Chunk " + (Int(Floor(sp_x)) Sar 4) + ", " + (Int(Floor(sp_z)) Sar 4)
		m$ = ""
		If sp_fliegen Then m = m + "fliegen  "
		If sp_wasser Then m = m + "schwimmen  "
		If sp_boden Then m = m + "Boden  "
		Text 10, 46, "Chunks " + welt_chunks + "   in Arbeit " + ld_offen + "   Bauzeit " + bau_ms + " ms   " + m
		If ziel_treffer Then Text 10, 64, "Blick: " + bl_name(Welt_Block(ziel_x, ziel_y, ziel_z)) + " (" + ziel_x + ", " + ziel_y + ", " + ziel_z + ")"
	EndIf
	Text 10, GraphicsHeight() - 24, "Block " + (gewaehlt + 1) + ": " + bl_name(auswahl(gewaehlt))
	If gefangen
		Rect GraphicsWidth() / 2 - 8, GraphicsHeight() / 2 - 1, 17, 3
		Rect GraphicsWidth() / 2 - 1, GraphicsHeight() / 2 - 8, 3, 17
	ElseIf Not rundflug
		Text GraphicsWidth() / 2, GraphicsHeight() / 2, "Klick ins Fenster zum Spielen - Tab gibt die Maus frei, Esc beendet", True, True
	EndIf
	Flip
Wend
End

.auswahl_daten
Data B_STEIN, B_ERDE, B_GRAS, B_SAND, B_KIES, B_STAMM, B_LAUB, B_SANDSTEIN, B_SCHNEE
