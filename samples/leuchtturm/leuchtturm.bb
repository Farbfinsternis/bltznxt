; Leuchtturm - ein kleiner Arena-Shooter im Stil von Quake III, geschrieben
; als gewoehnliches Blitz3D-Programm. Konzept: LEUCHTTURM.md.
;
; Stand: Schritt 4 - Waffen und Zielscheiben in der Platzhalter-Arena
; (daten/arena.glb von werkzeug/arena.py, Waffen und Scheiben von
; werkzeug/waffen.py). Eine Karte aus Blender ersetzt sie ohne
; Codeaenderung, wenn sie den Namensregeln folgt.
;
; Steuerung:  Klick ins Fenster faengt die Maus, Tab oder Esc gibt sie frei;
;             Esc bei freier Maus beendet.
;             Maus schauen   W A S D / Pfeile laufen   Leertaste springen
;             linke Maustaste feuern   1 2 3 / Mausrad Waffe waehlen
;             F1 Anzeige
;
; Rocket-Jump: nach unten schauen, springen und im selben Moment feuern.
;
; Die Maus wird nur gefangen, solange man spielt: zum Umsehen setzt das
; Programm sie in jedem Bild in die Fenstermitte, und das darf nicht
; passieren, bevor man ins Fenster geklickt hat.
;
; Die Welt laeuft in festen Takten zu 1/60 s, unabhaengig von der Bildrate:
; Bewegung und Kollision sind so bei jedem Rechner dieselben, und glTF-
; Animationen zaehlen ebenfalls 60 Bilder je Sekunde.

Include "karte.bb"
Include "spieler.bb"
Include "effekte.bb"
Include "ziele.bb"
Include "waffen.bb"

Const MAUS_EMPF# = 0.15     ; Grad je Pixel

Graphics3D 1024, 768, 0, 2
SetBuffer BackBuffer()

AmbientLight 90, 90, 100
sonne = CreateLight(1)
LightColor sonne, 230, 220, 200
RotateEntity sonne, 55, -35, 0

Karte_Laden("daten/arena.glb")
If karte_spawn
	sx# = EntityX(karte_spawn, True) : sy# = EntityY(karte_spawn, True) : sz# = EntityZ(karte_spawn, True)
	sg# = EntityYaw(karte_spawn, True)
Else
	sx = 0 : sy = 1 : sz = 0 : sg = 0
EndIf
ich.Spieler = Spieler_Neu(sx, sy, sz, sg)
CameraClsColor ich\kamera, 110, 150, 200
Effekte_Laden()
Ziele_Laden("daten/ziel.glb")
Waffen_Laden(ich, "daten")

anzeige = True
sprung_merken = False
gefangen = False
abzug_frei = False          ; der Klick, der die Maus faengt, schiesst nicht

zeit = MilliSecs()
rest# = 0
bilder = 0 : bilder_zeit = MilliSecs() : fps = 0

ende = False
While Not ende
	If KeyHit(59) Then anzeige = Not anzeige

	; Maus fangen und freigeben
	If gefangen
		If KeyHit(15) Or KeyHit(1)
			gefangen = False
			ShowPointer
		EndIf
	Else
		If KeyHit(1) Then ende = True
		If MouseHit(1)
			gefangen = True
			abzug_frei = False
			HidePointer
			MoveMouse GraphicsWidth() / 2, GraphicsHeight() / 2
			MouseXSpeed() : MouseYSpeed()
		EndIf
	EndIf

	vor# = 0 : seit# = 0 : abzug = False
	If gefangen
		If KeyHit(57) Then sprung_merken = True
		If MouseDown(1)
			abzug = abzug_frei
		Else
			abzug_frei = True
		EndIf
		For i = 1 To W_ANZAHL
			If KeyHit(1 + i) Then Waffe_Waehlen(i)
		Next
		rad = MouseZSpeed()
		If rad > 0 Then Waffe_Blaettern(1)
		If rad < 0 Then Waffe_Blaettern(-1)
		; Schauen einmal je Bild, nicht je Takt - die Maus soll sofort folgen.
		Spieler_Schauen(ich, -MouseXSpeed() * MAUS_EMPF, MouseYSpeed() * MAUS_EMPF)
		MoveMouse GraphicsWidth() / 2, GraphicsHeight() / 2
		If KeyDown(17) Or KeyDown(200) Then vor = vor + 1
		If KeyDown(31) Or KeyDown(208) Then vor = vor - 1
		If KeyDown(32) Or KeyDown(205) Then seit = seit + 1
		If KeyDown(30) Or KeyDown(203) Then seit = seit - 1
	EndIf

	; Feste Takte nachholen; nach einer langen Pause hoechstens eine
	; Viertelsekunde, sonst rennt die Welt davon.
	jetzt = MilliSecs()
	rest = rest + (jetzt - zeit) / 1000.0
	zeit = jetzt
	If rest > 0.25 Then rest = 0.25
	While rest >= TAKT
		Spieler_Takt(ich, vor, seit, sprung_merken)
		sprung_merken = False
		Raketen_Fliegen()
		Ziele_Takt()
		UpdateWorld
		Spieler_Nach(ich)
		Raketen_Nach(ich)
		Waffen_Takt(ich, abzug)
		Effekte_Takt()
		rest = rest - TAKT
	Wend

	; Aus der Welt gefallen: zurueck zum Start.
	If EntityY(ich\koerper) < -50 Then Spieler_Setzen(ich, sx, sy, sz, sg)

	RenderWorld

	bilder = bilder + 1
	If MilliSecs() - bilder_zeit >= 1000
		fps = bilder : bilder = 0 : bilder_zeit = MilliSecs()
	EndIf
	If anzeige
		Color 255, 255, 255
		Text 10, 10, "Leuchtturm - Schritt 4: Waffen   (F1 Anzeige, Tab Maus frei)"
		Text 10, 30, "Tempo " + Int(Spieler_Tempo(ich) * 10) / 10.0 + " m/s   " + fps + " fps"
		If ich\boden Then b$ = "Boden" Else b$ = "Luft"
		Text 10, 50, "x " + Int(EntityX(ich\koerper)) + "  y " + Int(EntityY(ich\koerper) - SP_HALB) + "  z " + Int(EntityZ(ich\koerper)) + "   " + b
		Text 10, 70, "Treffer " + zl_treffer + "   Scheiben zerstoert " + zl_zerstoert + "   Schuesse " + w_schuesse
	EndIf
	; Waffen und Munition, die gewaehlte hervorgehoben
	x = 10
	For i = 1 To W_ANZAHL
		If i = w_wunsch Then Color 255, 230, 120 Else Color 150, 150, 150
		t$ = i + " " + w_name(i) + " " + w_munition(i)
		Text x, GraphicsHeight() - 24, t
		x = x + StringWidth(t) + 24
	Next
	If gefangen
		; Fadenkreuz
		Color 255, 255, 255
		Rect GraphicsWidth() / 2 - 1, GraphicsHeight() / 2 - 1, 3, 3
	Else
		Color 255, 255, 255
		Text GraphicsWidth() / 2, GraphicsHeight() / 2, "Klick ins Fenster zum Spielen - Esc beendet", True, True
	EndIf
	Flip
Wend
End
