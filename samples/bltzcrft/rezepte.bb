; BLTZCRFT - Rezepte, Herstellen und Reparieren
;
; Survival-Handwerk wie in Conan Exiles oder Enshrouded statt des
; Minecraft-Gitters (CRAFTING.md): Im Inventar steht rechts eine Liste der
; bekannten Rezepte. Ein Klick zieht die Zutaten ab und reiht einen Auftrag
; in die Warteschlange (hoechstens RZ_SCHLANGE); der vorderste laeuft ueber
; die Zeit, auch bei geschlossenem Inventar. Ein Balken zeigt den
; Fortschritt, im Inventar und neben der Schnellleiste. Rechtsklick auf
; einen Auftrag bricht ihn ab und gibt die Zutaten zurueck. Ist das Inventar
; fuer das Ergebnis zu voll, wartet der fertige Auftrag, bis Platz ist.
;
; Freischalten wie in Enshrouded: Ein Rezept ist bekannt, sobald man jede
; seiner Zutaten schon einmal aufgehoben hat (dg_gesehen, dinge.bb) - ein
; Ast allein genuegt fuer die Steinaxt nicht, Ast und Steinbrocken schon.
;
; Werkbank: Rezepte mit rz_station = RZ_WERKBANK lassen sich nur bestellen,
; wenn eine Werkbank in der Naehe steht (rz_werkbank_nah, setzt das Spiel),
; und ihr Auftrag laeuft nur dort - geht man weg, haelt der Balken an. An
; der Werkbank lassen sich ausserdem abgenutzte und kaputte Werkzeuge
; reparieren: gegen die Haelfte ihrer Zutaten (abgerundet, mindestens 1 je
; Art) bekommen sie die volle Haltbarkeit zurueck.

Const RZ_MAX = 16
Const RZ_ZUTATEN = 4
Const RZ_SCHLANGE = 8
Const RZ_HAND = 0
Const RZ_WERKBANK = 1
Const RZ_REPARATUR_ZEIT# = 3.0

Dim rz_ergebnis(RZ_MAX - 1), rz_menge(RZ_MAX - 1), rz_zeit#(RZ_MAX - 1)
Dim rz_station(RZ_MAX - 1), rz_frei(RZ_MAX - 1)
Dim rz_zutat(RZ_MAX - 1, RZ_ZUTATEN - 1), rz_zmenge(RZ_MAX - 1, RZ_ZUTATEN - 1)
Global rz_anzahl

; Auftraege, vorn = 0: Rezeptnummer; bei einer Reparatur das Rezept des
; Werkzeugs und dessen alte Haltbarkeit
Dim rz_auftrag(RZ_SCHLANGE - 1), rz_auftrag_rep(RZ_SCHLANGE - 1), rz_auftrag_h(RZ_SCHLANGE - 1)
Global rz_auftraege
Global rz_fortschritt#              ; Sekunden am vordersten Auftrag
Global rz_meldung$                  ; fuer die Meldung im Bild (bltzcrft.bb)
Global rz_werkbank_nah              ; steht man an einer Werkbank?

; Ein Rezept: Ergebnis und Menge, Sekunden, Station, bis zu vier Zutaten.
Function Rezept(ergebnis, menge, zeit#, station, z1, m1, z2 = 0, m2 = 0, z3 = 0, m3 = 0, z4 = 0, m4 = 0)
	r = rz_anzahl
	rz_anzahl = rz_anzahl + 1
	rz_ergebnis(r) = ergebnis : rz_menge(r) = menge : rz_zeit(r) = zeit
	rz_station(r) = station : rz_frei(r) = False
	rz_zutat(r, 0) = z1 : rz_zmenge(r, 0) = m1
	rz_zutat(r, 1) = z2 : rz_zmenge(r, 1) = m2
	rz_zutat(r, 2) = z3 : rz_zmenge(r, 2) = m3
	rz_zutat(r, 3) = z4 : rz_zmenge(r, 3) = m4
End Function

; Nach Dinge_Arten. Die Liste aus CRAFTING.md.
Function Rezepte_Laden()
	rz_anzahl = 0
	Rezept(G_AST, 4, 2, RZ_HAND, B_STAMM, 1)
	Rezept(B_BRETTER, 4, 2, RZ_HAND, B_STAMM, 1)
	Rezept(G_BROCKEN, 2, 2, RZ_HAND, B_STEIN, 1)
	Rezept(G_STEINAXT, 1, 4, RZ_HAND, G_AST, 2, G_BROCKEN, 3)
	Rezept(G_STEINHACKE, 1, 4, RZ_HAND, G_AST, 2, G_BROCKEN, 3)
	Rezept(G_STEINSCHAUFEL, 1, 3, RZ_HAND, G_AST, 2, G_BROCKEN, 1)
	Rezept(B_FACKEL, 4, 2, RZ_HAND, G_AST, 1, G_KOHLE, 1)
	Rezept(B_WERKBANK, 1, 5, RZ_HAND, B_BRETTER, 4, G_BROCKEN, 2)
	Rezept(B_STEINZIEGEL, 4, 4, RZ_WERKBANK, B_STEIN, 4)
	rz_auftraege = 0 : rz_fortschritt = 0 : rz_meldung = "" : rz_werkbank_nah = False
End Function

; Neu frei gewordene Rezepte eintragen und melden. Jedes Bild aufrufen - es
; sind nur ein paar Rezepte.
Function Rezepte_Pruefen()
	neu$ = ""
	For r = 0 To rz_anzahl - 1
		If Not rz_frei(r)
			frei = True
			For k = 0 To RZ_ZUTATEN - 1
				If rz_zmenge(r, k) > 0
					If Not dg_gesehen(rz_zutat(r, k)) Then frei = False
				EndIf
			Next
			If frei
				rz_frei(r) = True
				If neu <> "" Then neu = neu + ", "
				neu = neu + dg_name(rz_ergebnis(r))
			EndIf
		EndIf
	Next
	If neu <> ""
		If Instr(neu, ",") Then rz_meldung = "Neue Rezepte: " + neu + " (E)" Else rz_meldung = "Neues Rezept: " + neu + " (E)"
	EndIf
End Function

; Das Rezept, nach dem Ding d entsteht (-1 = keins).
Function Rezept_Fuer(d)
	For r = 0 To rz_anzahl - 1
		If rz_ergebnis(r) = d Then Return r
	Next
	Return -1
End Function

; Wie viel von Zutat k kostet eine Reparatur nach Rezept r?
Function Reparatur_Menge(r, k)
	m = rz_zmenge(r, k) / 2
	If m < 1 Then m = 1
	Return m
End Function

; Zutatenmenge k fuer einen Auftrag: Rezept oder Reparatur.
Function Auftrag_Menge(r, k, rep)
	If rz_zmenge(r, k) = 0 Then Return 0
	If rep Then Return Reparatur_Menge(r, k)
	Return rz_zmenge(r, k)
End Function

; Wie oft liessen sich die Zutaten gerade aufbringen?
Function Rezept_Moeglich(r, rep = False)
	n = 9999
	For k = 0 To RZ_ZUTATEN - 1
		m = Auftrag_Menge(r, k, rep)
		If m > 0
			v = Inventar_Zaehlen(rz_zutat(r, k)) / m
			If v < n Then n = v
		EndIf
	Next
	Return n
End Function

Function Station_Da(station)
	If station = RZ_HAND Then Return True
	Return rz_werkbank_nah
End Function

; Bis zu n Auftraege fuer Rezept r anlegen: so viele, wie Zutaten und
; Warteschlange hergeben. Gibt die Zahl der angelegten zurueck.
Function Rezept_Herstellen(r, n = 1)
	If r < 0 Or r >= rz_anzahl Then Return 0
	If Not rz_frei(r) Then Return 0
	If Not Station_Da(rz_station(r))
		rz_meldung = dg_name(rz_ergebnis(r)) + " gibt es nur an einer Werkbank"
		Return 0
	EndIf
	k = Rezept_Moeglich(r)
	If n > k Then n = k
	If n > RZ_SCHLANGE - rz_auftraege Then n = RZ_SCHLANGE - rz_auftraege
	If n <= 0
		If rz_auftraege >= RZ_SCHLANGE Then rz_meldung = "Die Warteschlange ist voll" Else rz_meldung = "Dafuer fehlen Zutaten"
		Return 0
	EndIf
	For i = 1 To n
		For z = 0 To RZ_ZUTATEN - 1
			If rz_zmenge(r, z) > 0 Then Inventar_Entfernen(rz_zutat(r, z), rz_zmenge(r, z))
		Next
		rz_auftrag(rz_auftraege) = r : rz_auftrag_rep(rz_auftraege) = False : rz_auftrag_h(rz_auftraege) = 0
		rz_auftraege = rz_auftraege + 1
	Next
	Return n
End Function

; Ist das Werkzeug auf Platz i reparierbar? Abgenutzt und mit Rezept.
Function Reparierbar(i)
	If inv_anzahl(i) = 0 Then Return False
	d = inv_ding(i)
	If dg_haltbar(d) = 0 Or inv_haltbar(i) >= dg_haltbar(d) Then Return False
	Return Rezept_Fuer(d) >= 0
End Function

; Das Werkzeug auf Platz i zur Reparatur geben: es verlaesst das Inventar
; samt Kosten und kommt nach RZ_REPARATUR_ZEIT heil zurueck.
Function Reparieren(i)
	If Not Reparierbar(i) Then Return False
	If Not rz_werkbank_nah
		rz_meldung = "Reparieren geht nur an einer Werkbank"
		Return False
	EndIf
	r = Rezept_Fuer(inv_ding(i))
	If rz_auftraege >= RZ_SCHLANGE
		rz_meldung = "Die Warteschlange ist voll"
		Return False
	EndIf
	If Rezept_Moeglich(r, True) < 1
		rz_meldung = "Fuer die Reparatur fehlen Zutaten"
		Return False
	EndIf
	For z = 0 To RZ_ZUTATEN - 1
		m = Auftrag_Menge(r, z, True)
		If m > 0 Then Inventar_Entfernen(rz_zutat(r, z), m)
	Next
	rz_auftrag(rz_auftraege) = r : rz_auftrag_rep(rz_auftraege) = True : rz_auftrag_h(rz_auftraege) = inv_haltbar(i)
	rz_auftraege = rz_auftraege + 1
	inv_anzahl(i) = 0 : inv_ding(i) = B_LUFT : inv_haltbar(i) = 0
	Return True
End Function

Function Auftrag_Zeit#(a)
	If rz_auftrag_rep(a) Then Return RZ_REPARATUR_ZEIT
	Return rz_zeit(rz_auftrag(a))
End Function

Function Auftrag_Station(a)
	If rz_auftrag_rep(a) Then Return RZ_WERKBANK
	Return rz_station(rz_auftrag(a))
End Function

Function Auftrag_Entfernen(a)
	For i = a To rz_auftraege - 2
		rz_auftrag(i) = rz_auftrag(i + 1)
		rz_auftrag_rep(i) = rz_auftrag_rep(i + 1)
		rz_auftrag_h(i) = rz_auftrag_h(i + 1)
	Next
	rz_auftraege = rz_auftraege - 1
	If a = 0 Then rz_fortschritt = 0
End Function

; Auftrag a abbrechen: die Zutaten kommen zurueck, bei einer Reparatur auch
; das Werkzeug, wie es war. Passt nicht alles hinein, bleibt er stehen.
Function Auftrag_Abbrechen(a)
	If a < 0 Or a >= rz_auftraege Then Return False
	r = rz_auftrag(a) : rep = rz_auftrag_rep(a)
	For z = 0 To RZ_ZUTATEN - 1
		m = Auftrag_Menge(r, z, rep)
		If m > 0
			If Not Inventar_Passt(rz_zutat(r, z), m)
				rz_meldung = "Kein Platz fuer die Zutaten"
				Return False
			EndIf
		EndIf
	Next
	If rep
		If Not Inventar_Passt(rz_ergebnis(r), 1)
			rz_meldung = "Kein Platz fuer das Werkzeug"
			Return False
		EndIf
		Inventar_Hinzu(rz_ergebnis(r), 1, rz_auftrag_h(a))
	EndIf
	For z = 0 To RZ_ZUTATEN - 1
		m = Auftrag_Menge(r, z, rep)
		If m > 0 Then Inventar_Hinzu(rz_zutat(r, z), m)
	Next
	Auftrag_Entfernen(a)
	Return True
End Function

; Laeuft der vorderste Auftrag gerade? Nicht, wenn seine Station fehlt.
Function Auftrag_Laeuft()
	If rz_auftraege = 0 Then Return False
	Return Station_Da(Auftrag_Station(0))
End Function

; Die Zeit laufen lassen. Gibt True zurueck, wenn ein Auftrag fertig wurde.
Function Rezepte_Takt(dt#)
	If Not Auftrag_Laeuft() Then Return False
	r = rz_auftrag(0)
	rz_fortschritt = rz_fortschritt + dt
	If rz_fortschritt < Auftrag_Zeit(0) Then Return False
	rz_fortschritt = Auftrag_Zeit(0)
	If rz_auftrag_rep(0) Then menge = 1 Else menge = rz_menge(r)
	If Not Inventar_Passt(rz_ergebnis(r), menge)
		If rz_meldung = "" Then rz_meldung = "Inventar voll - " + dg_name(rz_ergebnis(r)) + " wartet"
		Return False
	EndIf
	Inventar_Hinzu(rz_ergebnis(r), menge)
	If rz_auftrag_rep(0) Then rz_meldung = dg_name(rz_ergebnis(r)) + " ist repariert"
	Auftrag_Entfernen(0)
	Return True
End Function

; Anteil 0..1 des vordersten Auftrags.
Function Rezepte_Anteil#()
	If rz_auftraege = 0 Then Return 0
	Return rz_fortschritt / Auftrag_Zeit(0)
End Function

; ---- Die Liste: Zeilen ------------------------------------------------------
;
; Erst die Handarbeit, dann - sobald es dort etwas gibt - der Abschnitt
; "An der Werkbank" mit ihren Rezepten und je einer Zeile pro reparierbarem
; Werkzeug im Inventar.

Const ZL_REZEPT = 0
Const ZL_TITEL = 1
Const ZL_REPARATUR = 2
Const ZL_MAX = 48

Dim zl_art(ZL_MAX - 1), zl_wert(ZL_MAX - 1)
Global zl_anzahl

Function Zeile_Neu(art, wert)
	If zl_anzahl >= ZL_MAX Then Return
	zl_art(zl_anzahl) = art : zl_wert(zl_anzahl) = wert
	zl_anzahl = zl_anzahl + 1
End Function

Function Zeilen_Bauen()
	zl_anzahl = 0
	For r = 0 To rz_anzahl - 1
		If rz_frei(r) And rz_station(r) = RZ_HAND Then Zeile_Neu(ZL_REZEPT, r)
	Next
	titel = zl_anzahl
	For r = 0 To rz_anzahl - 1
		If rz_frei(r) And rz_station(r) = RZ_WERKBANK
			If zl_anzahl = titel Then Zeile_Neu(ZL_TITEL, 0)
			Zeile_Neu(ZL_REZEPT, r)
		EndIf
	Next
	For i = 0 To INV_PLAETZE - 1
		If Reparierbar(i)
			If zl_anzahl = titel Then Zeile_Neu(ZL_TITEL, 0)
			Zeile_Neu(ZL_REPARATUR, i)
		EndIf
	Next
End Function

; Klick auf Zeile z; mehr = Umschalt (so oft wie moeglich).
Function Zeile_Klick(z, mehr)
	If z < 0 Or z >= zl_anzahl Then Return
	Select zl_art(z)
	Case ZL_REZEPT
		If mehr Then Rezept_Herstellen(zl_wert(z), RZ_SCHLANGE) Else Rezept_Herstellen(zl_wert(z), 1)
	Case ZL_REPARATUR
		Reparieren(zl_wert(z))
	End Select
End Function

; ---- Zeichnen und Klicks ---------------------------------------------------

Const RZ_BREITE = 372
Const RZ_ZEILE = 46
Const RZ_TITEL = 26                 ; Hoehe der Zeile "An der Werkbank"
Const RZ_FELD = 44                  ; Abstand der Auftraege

Function Zeile_Hoehe(z)
	If zl_art(z) = ZL_TITEL Then Return RZ_TITEL
	Return RZ_ZEILE
End Function

Function Zeilen_Hoehe()
	h = 0
	For z = 0 To zl_anzahl - 1 : h = h + Zeile_Hoehe(z) : Next
	If h = 0 Then h = RZ_ZEILE
	Return h
End Function

Function Rezepte_Tiefe()
	Return 36 + Zeilen_Hoehe() + 30 + SYMBOL_GROESSE + 30
End Function

; Die Liste steht rechts neben dem Inventar, oben buendig - ist sie zu
; lang, rueckt sie hoeher.
Function Rezepte_Links()
	Return Platz_X(0) - 14 + INV_LEISTE * INV_FELD + 24 + 12
End Function
Function Rezepte_Oben()
	y = Platz_Y(INV_LEISTE) - 40
	If y + Rezepte_Tiefe() > GraphicsHeight() - 8 Then y = GraphicsHeight() - 8 - Rezepte_Tiefe()
	If y < 8 Then y = 8
	Return y
End Function

Function Rezepte_Schlange_Y()
	Return Rezepte_Oben() + 36 + Zeilen_Hoehe() + 30
End Function

; Unter dem Mauszeiger: Zeile (-1 = keine) bzw. Auftrag (-1 = keiner).
Function Zeile_Unter(mx, my)
	x = Rezepte_Links() : y = Rezepte_Oben() + 36
	If mx < x + 4 Or mx >= x + RZ_BREITE - 4 Then Return -1
	For z = 0 To zl_anzahl - 1
		h = Zeile_Hoehe(z)
		If my >= y And my < y + h
			If zl_art(z) = ZL_TITEL Then Return -1
			Return z
		EndIf
		y = y + h
	Next
	Return -1
End Function
Function Auftrag_Unter(mx, my)
	x = Rezepte_Links() + 12 : y = Rezepte_Schlange_Y()
	If my < y Or my >= y + SYMBOL_GROESSE Or mx < x Then Return -1
	a = (mx - x) / RZ_FELD
	If a >= rz_auftraege Or (mx - x) Mod RZ_FELD >= SYMBOL_GROESSE Then Return -1
	Return a
End Function

Function Balken_Zeichnen(x, y, breite, anteil#, laeuft = True)
	Color 0, 0, 0 : Rect x, y, breite, 5, 1
	If laeuft Then Color 90, 200, 90 Else Color 200, 160, 60
	Rect x + 1, y + 1, Int((breite - 2) * anteil), 3, 1
End Function

; Zutaten einer Zeile: genug gruen, zu wenig rot, mit dem Vorrat dahinter.
Function Zutaten_Zeichnen(r, rep, x, y)
	For k = 0 To RZ_ZUTATEN - 1
		m = Auftrag_Menge(r, k, rep)
		If m > 0
			d = rz_zutat(r, k) : hab = Inventar_Zaehlen(d)
			s$ = m + " " + dg_name(d) + " (" + hab + ")"
			If hab >= m Then Color 140, 220, 140 Else Color 230, 120, 110
			Text x, y, s
			x = x + StringWidth(s) + 14
		EndIf
	Next
End Function

Function Zahl$(f#)
	Return Replace(Replace(Str(Int(f * 10) / 10.0), ".0", ""), ".", ",")
End Function

Function Rezepte_Zeichnen()
	Zeilen_Bauen()
	x = Rezepte_Links() : y = Rezepte_Oben()
	tief = Rezepte_Tiefe()
	Color 40, 40, 40 : Rect x, y, RZ_BREITE, tief, 1
	Color 160, 160, 160 : Rect x, y, RZ_BREITE, tief, 0
	Color 255, 255, 255
	Text x + 14, y + 12, "Herstellen"
	Color 170, 170, 170
	Text_Rechts(x + RZ_BREITE - 14, y + 12, "Klick: 1   Umschalt+Klick: alle")
	mx = MouseX() : my = MouseY()
	ueber = Zeile_Unter(mx, my)
	If zl_anzahl = 0
		Color 200, 200, 200
		Text x + 14, y + 44, "Noch keine Rezepte - sammle Zutaten:"
		Text x + 14, y + 62, "Aeste aus Laub, Brocken von losen Steinen"
	EndIf
	zy = y + 36
	For z = 0 To zl_anzahl - 1
		Select zl_art(z)
		Case ZL_TITEL
			If rz_werkbank_nah Then Color 255, 220, 120 Else Color 150, 150, 150
			t$ = "An der Werkbank"
			If Not rz_werkbank_nah Then t = t + " (keine in der Naehe)"
			Text x + 14, zy + 7, t
			Color 90, 90, 90 : Line x + 14, zy + 22, x + RZ_BREITE - 14, zy + 22
		Case ZL_REZEPT, ZL_REPARATUR
			rep = (zl_art(z) = ZL_REPARATUR)
			If rep
				i = zl_wert(z) : r = Rezept_Fuer(inv_ding(i)) : e = inv_ding(i)
				titel$ = dg_name(e) + " reparieren"    ; der Zustand steht am Symbol
				zeit# = RZ_REPARATUR_ZEIT
				geht = rz_werkbank_nah And Rezept_Moeglich(r, True) > 0
			Else
				r = zl_wert(z) : e = rz_ergebnis(r)
				titel$ = dg_name(e)
				If rz_menge(r) > 1 Then titel = rz_menge(r) + " " + titel
				zeit# = rz_zeit(r)
				geht = Station_Da(rz_station(r)) And Rezept_Moeglich(r) > 0
			EndIf
			If z = ueber
				If geht Then Color 70, 80, 70 Else Color 70, 60, 60
				Rect x + 4, zy, RZ_BREITE - 8, RZ_ZEILE - 4, 1
			EndIf
			If rep
				Stapel_Zeichnen(e, 1, inv_haltbar(i), x + 10, zy + 1)
			ElseIf inv_symbol(e)
				DrawImage inv_symbol(e), x + 10, zy + 1
			EndIf
			If geht Then Color 255, 255, 255 Else Color 150, 150, 150
			Text x + 60, zy + 4, titel
			Color 170, 170, 170
			Text_Rechts(x + RZ_BREITE - 14, zy + 4, Zahl(zeit) + " s")
			Zutaten_Zeichnen(r, rep, x + 60, zy + 22)
		End Select
		zy = zy + Zeile_Hoehe(z)
	Next
	; die Warteschlange
	sy = Rezepte_Schlange_Y()
	Color 200, 200, 200
	If rz_auftraege = 0
		Text x + 14, sy - 20, "Nichts in Arbeit"
	Else
		Text x + 14, sy - 20, "In Arbeit (Rechtsklick bricht ab)"
	EndIf
	weg = Auftrag_Unter(mx, my)
	For a = 0 To RZ_SCHLANGE - 1
		ax = x + 12 + a * RZ_FELD
		Color 0, 0, 0 : Rect ax - 2, sy - 2, SYMBOL_GROESSE + 4, SYMBOL_GROESSE + 4, 1
		Color 90, 90, 90 : Rect ax, sy, SYMBOL_GROESSE, SYMBOL_GROESSE, 1
		If a < rz_auftraege
			e = rz_ergebnis(rz_auftrag(a))
			If inv_symbol(e) Then DrawImage inv_symbol(e), ax, sy
			If a = weg Then Color 255, 255, 255 : Rect ax - 2, sy - 2, SYMBOL_GROESSE + 4, SYMBOL_GROESSE + 4, 0
		EndIf
	Next
	If rz_auftraege > 0
		r = rz_auftrag(0)
		laeuft = Auftrag_Laeuft()
		Balken_Zeichnen(x + 12, sy + SYMBOL_GROESSE + 6, RZ_BREITE - 24, Rezepte_Anteil(), laeuft)
		t$ = dg_name(rz_ergebnis(r))
		If rz_auftrag_rep(0) Then t = "Reparatur " + t
		If laeuft Then t = t + " - noch " + Zahl(Auftrag_Zeit(0) - rz_fortschritt) + " s" Else t = t + " - wartet auf eine Werkbank"
		Color 200, 200, 200
		Text x + 14, sy + SYMBOL_GROESSE + 14, t
	EndIf
End Function

; Bei geschlossenem Inventar: der laufende Auftrag rechts neben der
; Schnellleiste, mit Balken (gelb, wenn er auf eine Werkbank wartet).
Function Rezepte_Leiste_Zeichnen()
	If rz_auftraege = 0 Then Return
	x = (GraphicsWidth() + INV_LEISTE * INV_FELD) / 2 + 14
	y = GraphicsHeight() - INV_FELD - 8 + 4
	e = rz_ergebnis(rz_auftrag(0))
	If inv_symbol(e) Then DrawImage inv_symbol(e), x, y - 4
	Balken_Zeichnen(x, y + SYMBOL_GROESSE - 2, SYMBOL_GROESSE, Rezepte_Anteil(), Auftrag_Laeuft())
	If rz_auftraege > 1
		Color 0, 0, 0 : Text x + SYMBOL_GROESSE + 7, y + 9, "+" + (rz_auftraege - 1)
		Color 255, 255, 255 : Text x + SYMBOL_GROESSE + 6, y + 8, "+" + (rz_auftraege - 1)
	EndIf
End Function
