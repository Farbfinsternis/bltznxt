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
;             linke Maustaste halten: abbauen (wie in Minecraft mit der
;             Hand: Erde und Holz geben einen Block, Stein und Erze erst mit
;             der Spitzhacke; Axt und Schaufel sind schneller;
;             Laub ab und zu einen Ast, lose Steine Steinbrocken)
;             rechte Maustaste Block setzen (Fackeln an Boden oder Wand,
;             64 Stueck zum Start)
;             1 - 9 / Mausrad Platz der Schnellleiste waehlen
;             E Inventar (Linksklick nehmen/ablegen, Rechtsklick halber
;             Stapel/ein Stueck, Umschalt+Klick zwischen Leiste und Vorrat)
;             F1 Hilfe  F3 Anzeige   F4 Zeitraffer   T Chat (im Mehrspieler)
;
; Ein Tag dauert 20 Minuten, mit Sonne, Mond, Sternen und ziehenden Wolken
; (himmel.bb). Das Spiel beginnt um 7 Uhr; -zeit 21.5 beginnt um 21:30.
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
; Mit -foto <gier> <neig> schaut das Programm ohne Titelbild in diese
; Richtung, speichert nach 8 s foto_<uhrzeit>.bmp und beendet sich - zum
; Pruefen des Himmels zusammen mit -zeit (bltzcrft.exe -zeit 18 -foto 90 10).
; Mit -fackeln stellt es dazu ringsum Fackeln auf, sobald man steht.
; Mit -pos x y z beginnt das Spiel an dieser Stelle statt am Strand - etwa
; in einem Gewoelbe (bltzcrft.exe -pos -2900 60 1500).
; Mit -abbau dazu haelt es ab 4 s die linke Maustaste und speichert schon
; bei 4,5 s ein Bild (foto_<uhrzeit>_a.bmp) - Hand, Risse, Bruchstuecke.
; Mit -werkzeuge liegen dazu Aeste, Steinbrocken und drei Steinwerkzeuge
; (neu, abgenutzt, kaputt) im Inventar, -wahl <n> waehlt Platz n der
; Schnellleiste, -inventar oeffnet das Inventar fuer das Bild, -herstellen
; reiht dazu drei Steinaexte in die Warteschlange, -werkbank stellt eine
; Werkbank zwei Bloecke voraus.
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

; Meldung unten im Bild - global, damit auch Funktionen sie setzen koennen
Global meldung$, meldung_bis

Include "rauschen.bb"
Include "bloecke.bb"
Include "dinge.bb"
Include "welt.bb"
Include "netz.bb"
Include "spieler.bb"
Include "laden.bb"
Include "inventar.bb"
Include "rezepte.bb"
Include "abbau.bb"
Include "himmel.bb"
Include "hand.bb"
Include "partikel.bb"
Include "mehrspieler.bb"

AppTitle "BLTZCRFT"
Graphics3D 1280, 720, 0, 2
SetBuffer BackBuffer()

SeedRnd MilliSecs()
saat = 2026
rundflug = False : mehr = False
name$ = "Spieler" + Rand(10, 99)
join$ = ""
startzeit# = 7
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
	Case "-zeit" : i = i + 1 : startzeit = Float(Wort(args, i))
	Case "-abbau" : foto_abbau = True
	Case "-fackeln" : foto_fackeln = True
	Case "-werkzeuge" : foto_werkzeuge = True
	Case "-inventar" : foto_inventar = True
	Case "-herstellen" : foto_herstellen = True
	Case "-werkbank" : foto_werkbank = True
	Case "-wahl" : i = i + 1 : foto_wahl = Int(Wort(args, i))
	Case "-pos"
		pos_x# = Float(Wort(args, i + 1)) : pos_y# = Float(Wort(args, i + 2)) : pos_z# = Float(Wort(args, i + 3))
		pos_da = True : i = i + 3
	Case "-foto" : foto = True : foto_gier# = Float(Wort(args, i + 1)) : foto_neig# = Float(Wort(args, i + 2)) : i = i + 2
	Default
		If Left(arg, 1) <> "-" Then saat = Int(arg)
	End Select
	i = i + 1
Forever

Bloecke_Laden()
Dinge_Arten()
Inventar_Symbole()
Inventar_Leeren()
Rezepte_Laden()
; Fackeln zum Start - bis es eine Werkbank gibt
Inventar_Hinzu(B_FACKEL, 64)
If foto_werkzeuge
	; zum Pruefen: Gegenstaende und Werkzeuge in drei Zustaenden
	Inventar_Hinzu(G_AST, 12)
	Inventar_Hinzu(G_BROCKEN, 9)
	Inventar_Hinzu(G_STEINAXT, 1)
	Inventar_Hinzu(G_STEINHACKE, 1, 37)
	Inventar_Hinzu(G_STEINSCHAUFEL, 1, 0)
	Inventar_Hinzu(B_STEIN, 8)
EndIf
If foto_wahl > 0 And foto_wahl <= INV_LEISTE Then inv_gewaehlt = foto_wahl - 1
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
Himmel_Laden(startzeit)
Partikel_Laden()
Hand_Laden()
If rundflug Then Hand_Zeigen(False)
Spieler_Abstellen(sx, sz)
If pos_da
	Laden_Umgebung(Floor(pos_x), Floor(pos_z), 3)
	Spieler_Setzen(pos_x, pos_y, pos_z)
EndIf

anzeige = True
gefangen = False
sprung_merken = False

; Titelbild stehen lassen, bis man klickt - der Klick faengt die Maus.
; Esc beendet schon hier.
ende = False
If foto Then sp_gier = foto_gier : sp_neig = foto_neig
If Not (rundflug Or mehr Or foto)
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
		; dieselbe Tageszeit wie beim Host
		If mp_zeit >= 0 Then hm_ms = mp_zeit : hm_tag = mp_tag
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

While Not ende
	If KeyHit(61) Then anzeige = Not anzeige
	If KeyHit(59) Then hilfe = Not hilfe : hinweis_bis = 0
	If KeyHit(62)
		; Zeitraffer nur allein - sonst liefe die Zeit bei jedem anders
		If mp_an
			meldung = "Zeitraffer gibt es nur allein" : meldung_bis = MilliSecs() + 2000
		Else
			hm_raffer = Not hm_raffer
			If hm_raffer Then meldung = "Zeitraffer: ein Tag in 20 Sekunden (F4 aus)" Else meldung = "Zeitraffer aus"
			meldung_bis = MilliSecs() + 2000
		EndIf
	EndIf
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
			klick1 = MouseHit(1) : klick2 = MouseHit(2)
			If klick1
				If p >= 0
					If umschalt Then Inventar_Schieben(p) Else Inventar_Links(p)
				Else
					; rechts die Rezepte (rezepte.bb): Klick stellt einmal her
					; oder repariert, Umschalt+Klick so oft, wie Zutaten und
					; Warteschlange reichen
					Zeile_Klick(Zeile_Unter(MouseX(), MouseY()), umschalt)
				EndIf
			EndIf
			If klick2
				If p >= 0 Then Inventar_Rechts(p) Else Auftrag_Abbrechen(Auftrag_Unter(MouseX(), MouseY()))
			EndIf
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

	; -foto -abbau: ab 4 s die linke Maustaste wie gehalten
	foto_halten = foto_abbau And foto_start > 0 And MilliSecs() - foto_start > 4000
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
	Himmel_Takt(bild_dt)

	; Abbauen und Setzen. Der anvisierte Block wird nicht hervorgehoben -
	; welchen man trifft, zeigen das Fadenkreuz und beim Abbauen die Risse.
	If Blick_Strahl()
		If (gefangen Or foto_halten) And (Not chat_offen)
			; Abbauen: halten, bis der Block bricht (abbau.bb)
			MouseHit(1)
			If MouseDown(1) Or foto_halten
				Hand_Schlagen()
				b = Welt_Block(ziel_x, ziel_y, ziel_z)
				If ab_pause <= 0 Then Partikel_Schlag(ziel_x, ziel_y, ziel_z, ziel_vx - ziel_x, ziel_vy - ziel_y, ziel_vz - ziel_z, b, bild_dt)
				; mit dem, was gerade in der Hand liegt (Werkzeug: dinge.bb)
				hd = Inventar_Gewaehlt() : hh = inv_haltbar(inv_gewaehlt)
				If Abbau_Halten(ziel_x, ziel_y, ziel_z, b, bild_dt, hd, hh)
					Block_Abbauen(ziel_x, ziel_y, ziel_z, hd, hh)
					warum$ = Ertrag_Hinweis(b, hd, hh)
					If warum <> "" And meldung <> "Inventar voll"
						meldung = warum : meldung_bis = MilliSecs() + 3000
					EndIf
					; jeder Block mit Haerte kostet das Werkzeug einen Punkt
					If bl_haerte(b) > 0
						If Werkzeug_Abnutzen(inv_gewaehlt)
							meldung = dg_name(hd) + " ist kaputt - an der Werkbank reparieren"
							meldung_bis = MilliSecs() + 3500
						EndIf
					EndIf
				EndIf
			Else
				Abbau_Loslassen()
			EndIf
			If MouseHit(2)
				b = Inventar_Gewaehlt()
				If Welt_Block(ziel_x, ziel_y, ziel_z) = B_WERKBANK And (Not (KeyDown(42) Or KeyDown(54)))
					; Rechtsklick auf die Werkbank oeffnet das Inventar mit ihren
					; Rezepten; mit Umschalt setzt man wie sonst einen Block daran
					inv_offen = True : gefangen = False
					ShowPointer
					MouseHit(1) : MouseHit(2)
					Abbau_Loslassen()
				ElseIf b = B_LUFT
					meldung = "Dieser Platz ist leer - erst Bloecke abbauen (linke Maustaste halten)"
					meldung_bis = MilliSecs() + 2500
				ElseIf Not Ist_Block(b)
					meldung = dg_name(b) + " laesst sich nicht setzen"
					meldung_bis = MilliSecs() + 2000
				Else
					; hohes Gras macht Platz, wie in Minecraft
					ort_x = ziel_vx : ort_y = ziel_vy : ort_z = ziel_vz
					If Welt_Block(ziel_x, ziel_y, ziel_z) = B_HOHESGRAS Then ort_x = ziel_x : ort_y = ziel_y : ort_z = ziel_z
					If b = B_FACKEL
						; am Boden aufrecht, an einer Wand schraeg, nie an der Decke -
						; und nur an einem festen Block
						If ort_x = ziel_x And ort_y = ziel_y And ort_z = ziel_z
							ny = 1                  ; anstelle von hohem Gras
							halt = bl_fest(Welt_Block(ort_x, ort_y - 1, ort_z))
						Else
							nx = ziel_vx - ziel_x : ny = ziel_vy - ziel_y : nz = ziel_vz - ziel_z
							halt = bl_fest(Welt_Block(ziel_x, ziel_y, ziel_z))
						EndIf
						frei = Welt_Block(ort_x, ort_y, ort_z)
						If ny < 0
							meldung = "An der Decke haelt keine Fackel" : meldung_bis = MilliSecs() + 2000
							ort_y = -1
						ElseIf (Not halt) Or (frei <> B_LUFT And frei <> B_HOHESGRAS)
							ort_y = -1
						ElseIf ny = 0
							b = Fackel_An_Wand(-nx, -nz)
						EndIf
					ElseIf bl_pflanze(b)
						If Not Pflanze_Haelt(b, Welt_Block(ort_x, ort_y - 1, ort_z))
							If b = B_SCHILF Then meldung = "Schilf waechst nur auf Gras, Erde, Sand oder Schilf" Else meldung = bl_name(b) + " waechst nur auf Gras oder Erde"
							meldung_bis = MilliSecs() + 2500
							ort_y = -1
						EndIf
					ElseIf Not Platz_Frei(ort_x, ort_y, ort_z)
						ort_y = -1
					EndIf
					If ort_y >= 0
						If Welt_Aendern(ort_x, ort_y, ort_z, b)
							Hand_Schlagen()
							Mp_Block(ort_x, ort_y, ort_z, b)
							Inventar_Nehmen(inv_gewaehlt)
						EndIf
					EndIf
				EndIf
			EndIf
		Else
			Abbau_Loslassen()
		EndIf
	Else
		Abbau_Loslassen()
		MouseHit(1) : MouseHit(2)
		; ins Leere schlagen geht auch
		If gefangen And (Not chat_offen) And MouseDown(1) Then Hand_Schlagen()
	EndIf
	Abbau_Zeigen()
	Hand_Takt(bild_dt)
	Partikel_Takt(bild_dt)
	; Herstellen laeuft weiter, auch bei geschlossenem Inventar
	; die Werkbank zaehlt bis 4 Bloecke um den Spieler (alle 8 Bilder geprueft)
	If (bilder Mod 8) = 0 Then rz_werkbank_nah = Werkbank_In_Naehe(Floor(sp_x), Floor(sp_y + 0.5), Floor(sp_z))
	Rezepte_Pruefen()
	Rezepte_Takt(bild_dt)
	If rz_meldung <> "" Then meldung = rz_meldung : meldung_bis = MilliSecs() + 3000 : rz_meldung = ""
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
		Text 10, 28, "x " + Int(Floor(sp_x)) + "  y " + Int(Floor(sp_y)) + "  z " + Int(Floor(sp_z)) + "   Chunk " + (Int(Floor(sp_x)) Sar 4) + ", " + (Int(Floor(sp_z)) Sar 4) + "   Tag " + (hm_tag + 1) + ", " + Himmel_Uhr()
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
	If Not (rundflug Or inv_offen) Then Rezepte_Leiste_Zeichnen()
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
		Rezepte_Zeichnen()
	ElseIf gefangen
		Rect GraphicsWidth() / 2 - 8, GraphicsHeight() / 2 - 1, 17, 3
		Rect GraphicsWidth() / 2 - 1, GraphicsHeight() / 2 - 8, 3, 17
	ElseIf Not rundflug
		Text GraphicsWidth() / 2, GraphicsHeight() / 2, "Klick ins Fenster zum Spielen - Tab gibt die Maus frei, Esc beendet", True, True
	EndIf
	If foto
		If foto_start = 0
			foto_start = MilliSecs()
			If foto_inventar Then inv_offen = True
			If foto_herstellen Then Rezepte_Pruefen() : Rezept_Herstellen(3, 3) : rz_fortschritt = 2.5
		EndIf
		If foto_werkbank And (Not foto_werkbank_da) And MilliSecs() - foto_start > 500 And sp_boden
			; eine Werkbank zwei Bloecke voraus (Blick nach +z) auf den Boden
			foto_werkbank_da = True
			tx = Floor(sp_x) : tz = Floor(sp_z) + 2
			For ty = Floor(sp_y) + 4 To Floor(sp_y) - 8 Step -1
				If bl_fest(Welt_Block(tx, ty - 1, tz)) And (Not bl_fest(Welt_Block(tx, ty, tz)))
					Welt_Aendern(tx, ty, tz, B_WERKBANK)
					Exit
				EndIf
			Next
		EndIf
		If foto_fackeln And (Not foto_fackeln_da) And MilliSecs() - foto_start > 500 And sp_boden
			; ringsum auf den Boden, alle fuenf Bloecke
			foto_fackeln_da = True
			For fz = -10 To 10 Step 5
				For fx = -10 To 10 Step 5
					tx = Floor(sp_x) + fx : tz = Floor(sp_z) + fz
					For ty = Floor(sp_y) + 6 To Floor(sp_y) - 12 Step -1
						If Welt_Block(tx, ty, tz) = B_LUFT And bl_fest(Welt_Block(tx, ty - 1, tz))
							Welt_Aendern(tx, ty, tz, B_FACKEL)
							Exit
						EndIf
					Next
				Next
			Next
		EndIf
		If foto_abbau And (Not foto_a) And MilliSecs() - foto_start > 4500
			foto_a = True
			SaveBuffer BackBuffer(), "foto_" + Replace(Himmel_Uhr(), ":", "") + "_a.bmp"
		EndIf
		If MilliSecs() - foto_start > 8000
			uhr$ = Replace(Himmel_Uhr(), ":", "")
			SaveBuffer BackBuffer(), "foto_" + uhr + ".bmp"
			ende = True
		EndIf
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
Data 17
Data "Maus", "umsehen"
Data "W A S D / Pfeile", "laufen"
Data "Leertaste", "springen, im Wasser schwimmen"
Data "Strg", "rennen"
Data "F", "fliegen ein/aus (Leertaste hoch, Umschalt runter)"
Data "Linke Maustaste", "halten: abbauen (Stein, Erze nur mit Spitzhacke)"
Data "Rechte Maustaste", "setzen - auf eine Werkbank: ihre Rezepte"
Data "1 - 9 / Mausrad", "Platz der Schnellleiste waehlen"
Data "E", "Inventar oeffnen/schliessen"
Data "  im Inventar", "Links: Stapel nehmen/ablegen, Rechts: halber Stapel"
Data "", "bzw. ein Stueck, Umschalt+Links: Leiste <-> Vorrat"
Data "  Rezepte rechts", "Klick: herstellen, Umschalt: alle, Rechts: abbrechen"
Data "Tab", "Maus freigeben (Klick ins Fenster faengt sie wieder)"
Data "F3", "Anzeige mit Position, Uhrzeit und Leistung"
Data "F4", "Zeitraffer ein/aus (ein Tag in 20 Sekunden)"
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

; Den Block bei x,y,z abbauen: Bruchstuecke, Ertrag ins Inventar, an alle
; schicken. Was an ihm hing, bricht mit.
Function Block_Abbauen(x, y, z, d = 0, h = 0)
	b = Welt_Block(x, y, z)
	If Not Welt_Aendern(x, y, z, B_LUFT) Then Return
	Mp_Block(x, y, z, B_LUFT)
	Partikel_Bruch(x, y, z, b)
	; ist das Inventar voll, geht der Block verloren
	If Inventar_Hinzu(Block_Ertrag(b, d, h), Block_Ertrag_Anzahl(b)) > 0
		meldung = "Inventar voll" : meldung_bis = MilliSecs() + 1500
	EndIf
	; was an diesem Block hing, verliert den Halt: Wandfackeln daneben,
	; Pflanzen und Fackeln darauf (ein Schilfhalm bricht so ganz)
	For k = 0 To 3
		dx = (k = 0) - (k = 1) : dz = (k = 2) - (k = 3)
		n = Welt_Block(x + dx, y, z + dz)
		If bl_fackel(n) = 2 And fk_wx(n) = -dx And fk_wz(n) = -dz Then Block_Abbauen(x + dx, y, z + dz)
	Next
	n = Welt_Block(x, y + 1, z)
	If bl_pflanze(n) Or bl_fackel(n) = 1 Then Block_Abbauen(x, y + 1, z)
End Function

; Steht im Wuerfel von 4 Bloecken um x,y,z eine Werkbank?
Function Werkbank_In_Naehe(x, y, z)
	For dy = -4 To 4
		If y + dy >= 0 And y + dy < HOEHE
			For dz = -4 To 4
				For dx = -4 To 4
					If Welt_Block(x + dx, y + dy, z + dz) = B_WERKBANK Then Return True
				Next
			Next
		EndIf
	Next
	Return False
End Function

; Maus fangen: Zeiger weg und in die Mitte, aufgelaufene Bewegung und Klicks
; verwerfen.
Function Maus_Fangen()
	HidePointer
	MoveMouse GraphicsWidth() / 2, GraphicsHeight() / 2
	MouseXSpeed() : MouseYSpeed() : MouseZSpeed()
	MouseHit(1) : MouseHit(2)
End Function
