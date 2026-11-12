; BLTZCRFT - Kinofahrt fuer ein Video: bltzcrft.exe [saat] -video ORDNER
;
; Das Spiel faehrt eine vorgegebene Route ab und speichert jedes Bild als BMP
; (ORDNER\f00000.bmp ...), mit festem Takt von 30 Bildern je Sekunde. Vor
; jedem Bild wird alles gebaut, was in Sichtweite fehlt - die Aufnahme ruckelt
; nicht, egal wie lange ein Bild dauert. Zusammensetzen: werkzeug/video_schnitt.py
; in Blender.
;
; Ablauf (Sekunden):
;     0 -  40  hoch ueber das Land, in sanften Bogen
;    40 -  65  tiefer, dicht ueber Baeume und Wasser
;    65 - ...  landen, sobald Land darunter ist
;   ... - 115  zu Fuss, mit Umschauen, Springen an Stufen, ein Stueck rennen
;   115 - 125  stehen bleiben und einmal rundum schauen
;   125 - ...  auf die hoechste Stelle in der Naehe fliegen und dort landen
;   ... - 195  nach Westen schauen: die Sonne geht unter
;
; Die Uhrzeit folgt der Aufnahmezeit, nicht der Rechenzeit: KN_UHR ist so
; gewaehlt, dass die Sonne bei rund 170 s den Horizont beruehrt (18 Uhr).

Const KN_BPS = 30
Const KN_ENDE# = 195.0
Const KN_UHR# = 14.6

Global kn_an, kn_ordner$, kn_t#, kn_bild
Global kn_zustand, kn_ab#               ; Zustand und seit wann (Sekunden)
Global kn_vor#, kn_hoch, kn_rennen
Global kn_ueber# = 24                   ; geglaettete Flughoehe ueber Grund
Global kn_wand#, kn_dreh#               ; so lange schon an einer Wand, Restdrehung
Global kn_zx#, kn_zz#, kn_zy#           ; Ziel des Aussichtsflugs
Global kn_blick0#

; Hoechster Grund voraus (mit Platz fuer Baumkronen)
Function Kino_Boden#(gier#)
	h# = MEER
	For d = 0 To 40 Step 8
		g# = Gelaende_Hoehe(sp_x - Sin(gier) * d, sp_z + Cos(gier) * d)
		If g > h Then h = g
	Next
	Return h + 5
End Function

; Fliegen in ueber_soll Bloecken ueber dem Grund voraus
Function Kino_Fliegen(ueber_soll#, neig#)
	sp_fliegen = True
	kn_ueber = kn_ueber + (ueber_soll - kn_ueber) * 0.02
	soll# = Kino_Boden(sp_gier) + kn_ueber
	v# = Klemmen((soll - sp_y) * 0.8, -7, 7)
	sp_kino_vy = sp_kino_vy + (v - sp_kino_vy) * 0.1
	kn_vor = 1
	sp_neig = sp_neig + (neig - sp_neig) * 0.03
End Function

Function Kino_Wechsel(z)
	kn_zustand = z : kn_ab = kn_t
	sp_kino_vy = 0
End Function

; Winkel nach -180..180
Function Kino_Winkel#(w#)
	While w > 180 : w = w - 360 : Wend
	While w < -180 : w = w + 360 : Wend
	Return w
End Function

; Blick-Versatz in die Bewegungsrichtung uebernehmen - das Bild bleibt gleich
Function Kino_Blick_Falten()
	sp_gier = sp_gier + sp_blick : sp_blick = 0
End Function

Function Kino_Takt(dt#)
	t# = kn_t
	s# = t - kn_ab
	kn_vor = 0 : kn_hoch = False : kn_rennen = False

	If kn_bild = 0
		; in der Luft beginnen
		Spieler_Setzen(sp_x, Kino_Boden(sp_gier) + kn_ueber, sp_z)
		sp_neig = 14 : sp_fliegen = True
		Kino_Wechsel(0)
	EndIf

	Select kn_zustand
	Case 0      ; hoch ueber das Land
		sp_gier = sp_gier + (Cos(t * 9) * 6.3 + Cos(t * 23) * 4) * dt
		Kino_Fliegen(24, 14)
		If t >= 40 Then Kino_Wechsel(1)

	Case 1      ; tief
		sp_gier = sp_gier + (Cos(t * 9) * 6.3 + Cos(t * 23) * 4) * dt
		Kino_Fliegen(5, 8)
		If t >= 65 Then Kino_Wechsel(2)

	Case 2      ; landen, sobald Land darunter ist
		land = Gelaende_Hoehe(sp_x, sp_z) > MEER + 1
		If land
			kn_vor = 0.5
			sp_kino_vy = sp_kino_vy + (-4 - sp_kino_vy) * 0.1
			sp_neig = sp_neig + (4 - sp_neig) * 0.05
			If sp_boden Or s > 15
				sp_fliegen = False
				Hand_Zeigen(True)
				Kino_Wechsel(3)
			EndIf
		Else
			Kino_Fliegen(4, 6)
		EndIf

	Case 3      ; zu Fuss
		kn_vor = 1
		kn_rennen = (s > 20 And s < 32)
		; an einer Stufe springen, im Wasser hochschwimmen
		kn_hoch = (sp_wand And sp_boden) Or sp_wasser
		If sp_wand Then kn_wand = kn_wand + dt Else kn_wand = kn_wand - dt
		If kn_wand < 0 Then kn_wand = 0
		If kn_wand > 0.6 And kn_dreh = 0 Then kn_dreh = 80 : kn_wand = 0
		; Stamm oder Laub in Kopfhoehe voraus: frueh abbiegen statt hineinzulaufen
		vx = Floor(sp_x - Sin(sp_gier) * 2) : vz = Floor(sp_z + Cos(sp_gier) * 2) : fy = Floor(sp_y)
		If kn_dreh = 0 And (bl_fest(Welt_Block(vx, fy + 1, vz)) Or bl_fest(Welt_Block(vx, fy + 2, vz))) Then kn_dreh = 70
		; vor dem Ozean abbiegen
		If kn_dreh = 0 And Gelaende_Hoehe(sp_x - Sin(sp_gier) * 6, sp_z + Cos(sp_gier) * 6) <= MEER Then kn_dreh = 100
		If kn_dreh > 0
			d# = 70 * dt : If d > kn_dreh Then d = kn_dreh
			sp_gier = sp_gier + d : kn_dreh = kn_dreh - d
		Else
			sp_gier = sp_gier + (Sin(t * 13) * 14 + Sin(t * 5) * 8) * dt
		EndIf
		; umschauen beim Gehen
		sp_blick = 55 * Sin(s * 9) * Klemmen((s - 6) / 3, 0, 1)
		sp_neig = sp_neig + (6 + 10 * Sin(s * 17) - sp_neig) * 0.05
		If t >= 115 Then kn_blick0 = sp_blick : Kino_Wechsel(4)

	Case 4      ; stehen und einmal rundum schauen
		kn_hoch = sp_wasser
		u# = Klemmen(s / 10, 0, 1)
		sp_blick = kn_blick0 + 360 * Glatt(u)
		sp_neig = sp_neig + (2 - sp_neig) * 0.05
		If s >= 10
			Kino_Blick_Falten()
			Kino_Ziel()
			Hand_Zeigen(False)
			Kino_Wechsel(5)
		EndIf

	Case 5      ; auf die Aussichtsstelle fliegen
		sp_fliegen = True
		dx# = kn_zx - sp_x : dz# = kn_zz - sp_z
		dist# = Sqr(dx * dx + dz * dz)
		If dist > 1.5
			w# = Kino_Winkel(ATan2(-dx, dz) - sp_gier)
			d# = 50 * dt
			If w > d Then w = d
			If w < -d Then w = -d
			sp_gier = sp_gier + w
		EndIf
		kn_vor = Klemmen(dist / 10, 0, 1) * Klemmen(s / 2, 0, 1)
		If dist > 1.5
			soll# = kn_zy + 7
			b# = Kino_Boden(sp_gier) + 3
			If b > soll Then soll = b
			v# = Klemmen((soll - sp_y) * 0.8, -6, 6)
		Else
			v = -3
		EndIf
		sp_kino_vy = sp_kino_vy + (v - sp_kino_vy) * 0.1
		sp_neig = sp_neig + (10 - sp_neig) * 0.03
		If (sp_boden And dist <= 2.5) Or t > 165
			sp_fliegen = False
			Kino_Wechsel(6)
		EndIf

	Case 6      ; nach Westen schauen
		kn_hoch = sp_wasser
		sp_gier = sp_gier + Kino_Winkel(90 - sp_gier) * 0.02
		sp_neig = sp_neig + (3 - sp_neig) * 0.02
	End Select
End Function

; Aussichtsstelle fuer den Sonnenuntergang: an Land, nichts ueber dem Kopf,
; nach Westen (-x) freie Sicht - kein Baum davor, kein hoeheres Gelaende.
; Hoeher ist besser, Wasser im Westen auch (dort spiegelt der Himmel);
; etwas naeher zaehlt etwas mehr. Ohne Treffer bleibt man, wo man ist.
Function Kino_Ziel()
	beste# = -9999
	kn_zx = sp_x : kn_zz = sp_z : kn_zy = Floor(sp_y)
	For dz = -72 To 72 Step 3
		For dx = -72 To 72 Step 3
			e# = Sqr(dx * dx + dz * dz)
			If e <= 72
				x = Floor(sp_x) + dx : z = Floor(sp_z) + dz
				g = Gelaende_Hoehe(x, z)
				If g > MEER + 1
					If Kino_Frei(x, g, z)
						nass = 0
						For d = 8 To 120 Step 8
							If Gelaende_Hoehe(x - d, z) <= MEER Then nass = nass + 1
						Next
						wert# = g + nass * 1.5 - e * 0.05
						If wert > beste
							beste = wert
							kn_zx = x + 0.5 : kn_zz = z + 0.5 : kn_zy = g
						EndIf
					EndIf
				EndIf
			EndIf
		Next
	Next
End Function

Function Kino_Frei(x, g, z)
	; ueber der Stelle und den Nachbarn kein Laub
	For y = g + 1 To g + 10
		For n = -1 To 1
			If bl_fest(Welt_Block(x + n, y, z)) Or bl_fest(Welt_Block(x, y, z + n)) Then Return False
		Next
	Next
	; nach Westen: nah kein Block in Augenhoehe, fern kein hoeheres Gelaende
	For d = 1 To 40
		For y = g + 1 To g + 4
			If bl_fest(Welt_Block(x - d, y, z)) Or bl_fest(Welt_Block(x - d, y, z - 1)) Or bl_fest(Welt_Block(x - d, y, z + 1)) Then Return False
		Next
	Next
	For d = 40 To 160 Step 4
		If Gelaende_Hoehe(x - d, z) > g + 1 + d * 0.04 Then Return False
	Next
	Return True
End Function

; Nach dem Zeichnen: Bild speichern, Uhr weiterstellen. Die Fahrt ist bei
; gleicher Saat Bild fuer Bild gleich - ein zweiter Lauf in denselben Ordner
; schreibt darum nur, was fehlt oder leer ist (Luecken fuellen). Schlaegt das
; Schreiben fehl (die Platte war beschaeftigt), wird es kurz darauf noch
; zweimal versucht; was dann noch fehlt, steht in kino_fehler.txt.
Function Kino_Bild()
	datei$ = kn_ordner + "\f" + Right("00000" + kn_bild, 5) + ".bmp"
	If FileSize(datei) = 0
		For versuch = 1 To 3
			If SaveBuffer(BackBuffer(), datei) Then Exit
			Delay 200
		Next
		If FileSize(datei) = 0
			f = OpenFile(kn_ordner + "\kino_fehler.txt")
			If f = 0 Then f = WriteFile(kn_ordner + "\kino_fehler.txt")
			If f Then SeekFile f, FileSize(kn_ordner + "\kino_fehler.txt") : WriteLine f, "Bild " + kn_bild + " nicht gespeichert" : CloseFile f
		EndIf
	EndIf
	kn_bild = kn_bild + 1
	kn_t = Float(kn_bild) / KN_BPS
End Function
