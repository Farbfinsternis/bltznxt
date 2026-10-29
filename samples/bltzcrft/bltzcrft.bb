; BLTZCRFT - eine Welt aus Bloecken im Stil von Minecraft, geschrieben als
; gewoehnliches Blitz3D-Programm und uebersetzt mit BLTZNXT.
;
; Stand: erste Fassung - die Welt entsteht in Chunks aus einer Saat
; (Gelaende, Ozean, Seen, Tuempel, Hoehlen, Erze, Baeume) und wird um den
; Spieler herum nachgeladen; man laeuft in der Ego-Sicht hindurch.
; Abgebaute Bloecke kommen ins Inventar und lassen sich wieder setzen;
; gespeichert wird nichts.
;
; Steuerung:  Klick ins Fenster faengt die Maus, Tab gibt sie frei, Esc beendet
;             Maus schauen   W A S D / Pfeile laufen   Leertaste springen,
;             schwimmen      Strg rennen     F fliegen (Leertaste hoch,
;             Umschalt runter)
;             linke Maustaste halten: abbauen  rechte Maustaste Block setzen
;             1 - 9 / Mausrad Platz der Schnellleiste waehlen
;             E Inventar (Linksklick nehmen/ablegen, Rechtsklick halber
;             Stapel/ein Stueck, Umschalt+Klick zwischen Leiste und Vorrat)
;             F1 Hilfe  F3 Anzeige   T Chat (im Mehrspieler)
;
; Mehrspieler: im Titelbild M druecken. Das Programm sucht im LAN ein
; laufendes BLTZCRFT, tritt bei und bekommt dessen Welt - sonst hostet es
; selbst. Die anderen erscheinen als Figur mit Namen, Bloecke und Chat gehen
; an alle (mehrspieler.bb). Aufruf: -mehrspieler startet gleich so, ohne
; Titelbild, -name Anna setzt den eigenen Namen, -join 192.168.0.5 tritt
; genau dort bei.
;
; Aufruf mit einer Zahl als Saat: bltzcrft.exe 1234 - ohne Zahl 2026.
; Mit -rundflug fliegt das Programm 20 s von selbst geradeaus, misst die
; Bildzeiten, gibt sie aus (auch in rundflug.txt) und beendet sich
; (bltzcrft.exe 2026 -rundflug).
; Mit -test <s> prueft das Programm den Mehrspieler ohne Maus und Tastatur:
; wer beitritt, schickt nach 3 s eine Chatzeile und setzt einen Stein
; zwischen sich und den Host; kurz vor Schluss speichert jeder ein Bild
; (test_<name>.bmp), am Ende schreibt er Chat und Aenderungen nach
; test_<name>.txt (bltzcrft.exe -test 12 -name Host).
;
; Protokoll der Stellen, an denen BLTZNXT die Arbeit erschwert hat:
; PROTOKOLL.md in diesem Ordner.

Const SICHT = 12            ; Sichtweite in Chunks
Const MAUS_EMPF# = 0.15     ; Grad je Pixel
Const BAU_ZEIT = 7          ; Millisekunden je Bild fuer Erzeugen und Bauen

Include "rauschen.bb"
Include "bloecke.bb"
Include "welt.bb"
Include "netz.bb"
Include "spieler.bb"
Include "laden.bb"
Include "inventar.bb"
Include "abbau.bb"
Include "mehrspieler.bb"

AppTitle "BLTZCRFT"
Graphics3D 1280, 720, 0, 2
SetBuffer BackBuffer()

SeedRnd MilliSecs()
saat = 2026
rundflug = False : mehr = False
name$ = "Spieler" + Rand(10, 99)
join$ = ""
args$ = Trim(CommandLine())
i = 1
Repeat
	arg$ = Wort(args, i)
	If arg = "" Then Exit
	Select Lower(arg)
	Case "-rundflug" : rundflug = True
	Case "-mehrspieler" : mehr = True
	Case "-name" : i = i + 1 : name = Wort(args, i)
	Case "-join" : i = i + 1 : join = Wort(args, i) : mehr = True
	Case "-test" : i = i + 1 : test_s = Int(Wort(args, i)) : mehr = True
	Default
		If Left(arg, 1) <> "-" Then saat = Int(arg)
	End Select
	i = i + 1
Forever

Bloecke_Laden()
Inventar_Symbole()
Inventar_Leeren()
Abbau_Laden()
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

Spieler_Neu(sx + 0.5, HOEHE, sz + 0.5)
Spieler_Abstellen(sx, sz)

anzeige = True
gefangen = False
sprung_merken = False

; Titelbild stehen lassen, bis man klickt - der Klick faengt die Maus.
; Esc beendet schon hier.
ende = False
If Not (rundflug Or mehr)
	FlushKeys : FlushMouse
	Repeat
		Laden_Bild("Klick: allein spielen - M: Mehrspieler im LAN - im Spiel zeigt F1 die Steuerung - Esc beendet", 1)
		If KeyHit(1) Then ende = True
		If KeyHit(50) Then mehr = True
		If MouseHit(1)
			gefangen = True
			HidePointer
			MoveMouse GraphicsWidth() / 2, GraphicsHeight() / 2
			MouseXSpeed() : MouseYSpeed()
		EndIf
	Until gefangen Or ende Or mehr
EndIf

; Mehrspieler: beitreten und die Welt des Hosts uebernehmen, sonst hosten
meldung$ = "" : meldung_bis = 0
If mehr And (Not ende)
	Laden_Bild("Suche ein Spiel im LAN ...", 1)
	r = Mp_Starten(name, join, saat)
	If r = 1
		If mp_saat <> saat
			saat = mp_saat
			Welt_Neu(saat)
		EndIf
		; vier Bloecke neben den Host stellen
		Laden_Bild("Lade die Welt des Hosts ...", 1)
		Spieler_Abstellen(Floor(mp_hx) + 4, Floor(mp_hz))
		sp_gier = 90 : sp_neig = 0      ; zum Host hin schauen (er steht bei -x)
		meldung = "Beigetreten - T: Chat" : meldung_bis = MilliSecs() + 4000
	ElseIf r = 2
		meldung = "Kein Spiel im LAN gefunden - du hostest, andere koennen beitreten (T: Chat)"
		meldung_bis = MilliSecs() + 5000
	Else
		meldung = "Mehrspieler ging nicht (Meldung auf der Konsole) - du spielst allein"
		meldung_bis = MilliSecs() + 5000
	EndIf
EndIf
If ld_logo > 0 Then FreeImage ld_logo

zeit = MilliSecs()
rest# = 0
bilder = 0 : bilder_zeit = MilliSecs() : fps = 0
bau_ms = 0
bild_dt# = 0
; Die Hinweiszeile ueber der Schnellleiste steht die ersten drei Minuten, bis
; man F1 drueckt - danach weiss man, wo die Hilfe ist.
hinweis_bis = MilliSecs() + 180000
hilfe = False

; dunkler Rahmen um den anvisierten Block
rahmen = CreateCube()
ScaleEntity rahmen, 0.505, 0.505, 0.505
EntityColor rahmen, 0, 0, 0
EntityAlpha rahmen, 0.25
EntityFX rahmen, 1
HideEntity rahmen

While Not ende
	If KeyHit(61) Then anzeige = Not anzeige
	If KeyHit(59) Then hilfe = Not hilfe : hinweis_bis = 0
	If hilfe
		; Esc schliesst erst die Hilfe (Blitz wertet beide Seiten von And aus -
		; ein "hilfe And KeyHit(1)" verschluckte Esc auch ohne Hilfe)
		If KeyHit(1) Then hilfe = False
	ElseIf inv_offen
		; Esc und E schliessen das Inventar und fangen die Maus wieder
		If KeyHit(1) Or KeyHit(18)
			Inventar_Schliessen()
			gefangen = True
			Maus_Fangen()
		Else
			umschalt = KeyDown(42) Or KeyDown(54)
			p = Platz_Unter(MouseX(), MouseY())
			If MouseHit(1) And p >= 0
				If umschalt Then Inventar_Schieben(p) Else Inventar_Links(p)
			EndIf
			If MouseHit(2) And p >= 0 Then Inventar_Rechts(p)
		EndIf
	ElseIf chat_offen
		; Esc und Enter schliessen den Chat (mehrspieler.bb); danach die Maus
		; wieder fangen, ihre Bewegung beim Tippen zaehlt nicht
		Chat_Tippen()
		If (Not chat_offen) And gefangen Then Maus_Fangen()
	Else
		If KeyHit(1) Then ende = True
		If KeyHit(20) And mp_an Then Chat_Oeffnen()
		If KeyHit(18) And (Not rundflug)
			inv_offen = True
			gefangen = False
			ShowPointer
			MouseHit(1) : MouseHit(2)
		ElseIf gefangen
			If KeyHit(15)
				gefangen = False
				ShowPointer
			EndIf
		ElseIf Not rundflug
			If MouseHit(1)
				gefangen = True
				Maus_Fangen()
			EndIf
		EndIf
	EndIf

	vor# = 0 : seit# = 0 : hoch = False : runter = False : rennen = False
	If gefangen And (Not chat_offen)
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
		For i = 0 To INV_LEISTE - 1
			If KeyHit(2 + i) Then inv_gewaehlt = i
		Next
		rad = MouseZSpeed()
		If rad < 0 Then inv_gewaehlt = (inv_gewaehlt + 1) Mod INV_LEISTE
		If rad > 0 Then inv_gewaehlt = (inv_gewaehlt + INV_LEISTE - 1) Mod INV_LEISTE
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
	bild_dt = (jetzt - zeit) / 1000.0
	rest = rest + bild_dt
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
		If gefangen And (Not chat_offen)
			; Abbauen: halten, bis der Block bricht (abbau.bb)
			MouseHit(1)
			If MouseDown(1)
				b = Welt_Block(ziel_x, ziel_y, ziel_z)
				If Abbau_Halten(ziel_x, ziel_y, ziel_z, b, bild_dt)
					If Welt_Aendern(ziel_x, ziel_y, ziel_z, B_LUFT)
						Mp_Block(ziel_x, ziel_y, ziel_z, B_LUFT)
						; ist das Inventar voll, geht der Block verloren
						If Inventar_Hinzu(Block_Ertrag(b), 1) > 0
							meldung = "Inventar voll" : meldung_bis = MilliSecs() + 1500
						EndIf
					EndIf
				EndIf
			Else
				Abbau_Loslassen()
			EndIf
			If MouseHit(2)
				b = Inventar_Gewaehlt()
				If b = B_LUFT
					meldung = "Dieser Platz ist leer - erst Bloecke abbauen (linke Maustaste halten)"
					meldung_bis = MilliSecs() + 2500
				ElseIf Platz_Frei(ziel_vx, ziel_vy, ziel_vz)
					If Welt_Aendern(ziel_vx, ziel_vy, ziel_vz, b)
						Mp_Block(ziel_vx, ziel_vy, ziel_vz, b)
						Inventar_Nehmen(inv_gewaehlt)
					EndIf
				EndIf
			EndIf
		Else
			Abbau_Loslassen()
		EndIf
	Else
		HideEntity rahmen
		Abbau_Loslassen()
		MouseHit(1) : MouseHit(2)
	EndIf
	Abbau_Zeigen()
	Mp_Takt(bild_dt)
	If test_s > 0
		If test_start = 0 Then test_start = MilliSecs()
		If (Not test_gesetzt) And MilliSecs() - test_start > 3000 And mp_an
			test_gesetzt = True
			If Not NetGameHost()
				SendNetMsg 4, "Hallo vom Test", mp_ich
				Chat_Zeile(mp_name + ": Hallo vom Test", 255, 255, 255)
				tx = Floor(sp_x) - 1 : ty = Floor(sp_y) : tz = Floor(sp_z)
				If Welt_Aendern(tx, ty, tz, B_STEIN) Then Mp_Block(tx, ty, tz, B_STEIN)
			EndIf
		EndIf
		If MilliSecs() - test_start > test_s * 1000 Then ende = True
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
		If mp_an
			n = 0 : For mm.Mitspieler = Each Mitspieler : n = n + 1 : Next
			rolle$ = "Mitspieler" : If NetGameHost() Then rolle = "Host"
			Text 10, 82, "Mehrspieler: " + mp_name + " (" + rolle + "), " + n + " weitere"
		EndIf
	EndIf
	Mp_Namen_Zeichnen(sp_kamera)
	Chat_Zeichnen()
	If test_s > 0 And (Not test_bild) And MilliSecs() - test_start > test_s * 1000 - 1500
		test_bild = True
		SaveBuffer BackBuffer(), "test_" + mp_name + ".bmp"
	EndIf
	If Not rundflug Then Leiste_Zeichnen()
	If MilliSecs() < hinweis_bis And (Not (inv_offen Or hilfe Or rundflug))
		Text_Schatten(GraphicsWidth() / 2, GraphicsHeight() - INV_FELD - 40, "Linke Maustaste halten: abbauen    Rechte Maustaste: setzen    E: Inventar    F1: Hilfe", 220, 220, 220)
	EndIf
	If MilliSecs() < meldung_bis
		Text_Schatten(GraphicsWidth() / 2, GraphicsHeight() - INV_FELD - 64, meldung, 255, 220, 120)
	EndIf
	Color 255, 255, 255
	If hilfe
		Hilfe_Zeichnen()
	ElseIf inv_offen
		Inventar_Zeichnen()
	ElseIf gefangen
		Rect GraphicsWidth() / 2 - 8, GraphicsHeight() / 2 - 1, 17, 3
		Rect GraphicsWidth() / 2 - 1, GraphicsHeight() / 2 - 8, 3, 17
	ElseIf Not rundflug
		Text GraphicsWidth() / 2, GraphicsHeight() / 2, "Klick ins Fenster zum Spielen - Tab gibt die Maus frei, Esc beendet", True, True
	EndIf
	Flip
Wend
If test_s > 0
	datei = WriteFile("test_" + mp_name + ".txt")
	If datei
		rolle$ = "allein" : If mp_an Then rolle = "Mitspieler" : If NetGameHost() Then rolle = "Host"
		WriteLine datei, mp_name + " (" + rolle + "), Saat " + saat
		For mm.Mitspieler = Each Mitspieler
			WriteLine datei, "sieht " + mm\name + " bei " + Int(mm\x) + " " + Int(mm\y) + " " + Int(mm\z)
		Next
		For zz.ChatZeile = Each ChatZeile
			WriteLine datei, "chat " + zz\text
		Next
		For aa.Aenderung = Each Aenderung
			WriteLine datei, "block " + aa\x + " " + aa\y + " " + aa\z + " " + bl_name(aa\b) + ", in der Welt " + bl_name(Welt_Block(aa\x, aa\y, aa\z))
		Next
		CloseFile datei
	EndIf
EndIf
Mp_Beenden()
End

; Mittig ausgerichteter Text mit dunklem Schatten, lesbar auch vor Himmel
; und Schnee.
Function Text_Schatten(x, y, s$, r, g, b)
	Color 0, 0, 0 : Text x + 1, y + 1, s, True, True
	Color r, g, b : Text x, y, s, True, True
End Function

; F1: alle Tasten auf einen Blick, ueber dem laufenden Spiel.
Function Hilfe_Zeichnen()
	Restore hilfe_daten
	Read n
	breite = 560 : tief = n * 20 + 64
	links = (GraphicsWidth() - breite) / 2 : oben = (GraphicsHeight() - tief) / 2 - 40
	Color 40, 40, 40 : Rect links, oben, breite, tief, 1
	Color 160, 160, 160 : Rect links, oben, breite, tief, 0
	Color 255, 255, 255
	Text links + 16, oben + 12, "Steuerung"
	Text_Rechts(links + breite - 16, oben + 12, "F1 / Esc schliesst")
	For i = 1 To n
		Read taste$, was$
		y = oben + 24 + i * 20
		Color 255, 220, 120 : Text links + 16, y, taste
		Color 230, 230, 230 : Text links + 200, y, was
	Next
End Function

.hilfe_daten
Data 15
Data "Maus", "umsehen"
Data "W A S D / Pfeile", "laufen"
Data "Leertaste", "springen, im Wasser schwimmen"
Data "Strg", "rennen"
Data "F", "fliegen ein/aus (Leertaste hoch, Umschalt runter)"
Data "Linke Maustaste", "halten: Block abbauen, er kommt ins Inventar"
Data "Rechte Maustaste", "Block aus der Schnellleiste setzen"
Data "1 - 9 / Mausrad", "Platz der Schnellleiste waehlen"
Data "E", "Inventar oeffnen/schliessen"
Data "  im Inventar", "Links: Stapel nehmen/ablegen, Rechts: halber Stapel"
Data "", "bzw. ein Stueck, Umschalt+Links: Leiste <-> Vorrat"
Data "Tab", "Maus freigeben (Klick ins Fenster faengt sie wieder)"
Data "F3", "Anzeige mit Position und Leistung"
Data "T", "Chat im Mehrspieler (Enter schickt, Esc verwirft)"
Data "Esc", "beenden"

; Spieler bei (x, z) auf den obersten festen Block stellen; erst die
; Umgebung erzeugen.
Function Spieler_Abstellen(x, z)
	Laden_Umgebung(x, z, 3)
	y = HOEHE - 1
	While y > 0 And (Not bl_fest(Welt_Block(x, y, z)))
		y = y - 1
	Wend
	Spieler_Setzen(x + 0.5, y + 1, z + 0.5)
End Function

; Maus fangen: Zeiger weg und in die Mitte, aufgelaufene Bewegung und Klicks
; verwerfen.
Function Maus_Fangen()
	HidePointer
	MoveMouse GraphicsWidth() / 2, GraphicsHeight() / 2
	MouseXSpeed() : MouseYSpeed() : MouseZSpeed()
	MouseHit(1) : MouseHit(2)
End Function
