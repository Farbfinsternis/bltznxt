; BLTZCRFT - Rezepte und Herstellen
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
; Rezepte mit Station (rz_station <> 0) gehoeren an die Werkbank und kommen
; mit ihr (Schritt 4).

Const RZ_MAX = 16
Const RZ_ZUTATEN = 4
Const RZ_SCHLANGE = 8
Const RZ_HAND = 0
Const RZ_WERKBANK = 1

Dim rz_ergebnis(RZ_MAX - 1), rz_menge(RZ_MAX - 1), rz_zeit#(RZ_MAX - 1)
Dim rz_station(RZ_MAX - 1), rz_frei(RZ_MAX - 1)
Dim rz_zutat(RZ_MAX - 1, RZ_ZUTATEN - 1), rz_zmenge(RZ_MAX - 1, RZ_ZUTATEN - 1)
Global rz_anzahl

Dim rz_auftrag(RZ_SCHLANGE - 1)     ; Rezeptnummer je Auftrag, vorn = 0
Global rz_auftraege
Global rz_fortschritt#              ; Sekunden am vordersten Auftrag
Global rz_meldung$                  ; fuer die Meldung im Bild (bltzcrft.bb)

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

; Nach Dinge_Arten. Die Liste aus CRAFTING.md, erst einmal von Hand.
Function Rezepte_Laden()
	rz_anzahl = 0
	Rezept(G_AST, 4, 2, RZ_HAND, B_STAMM, 1)
	Rezept(B_BRETTER, 4, 2, RZ_HAND, B_STAMM, 1)
	Rezept(G_BROCKEN, 2, 2, RZ_HAND, B_STEIN, 1)
	Rezept(G_STEINAXT, 1, 4, RZ_HAND, G_AST, 2, G_BROCKEN, 3)
	Rezept(G_STEINHACKE, 1, 4, RZ_HAND, G_AST, 2, G_BROCKEN, 3)
	Rezept(G_STEINSCHAUFEL, 1, 3, RZ_HAND, G_AST, 2, G_BROCKEN, 1)
	Rezept(B_FACKEL, 4, 2, RZ_HAND, G_AST, 1, G_KOHLE, 1)
	rz_auftraege = 0 : rz_fortschritt = 0 : rz_meldung = ""
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

; Wie oft liessen sich die Zutaten von Rezept r gerade aufbringen?
Function Rezept_Moeglich(r)
	n = 9999
	For k = 0 To RZ_ZUTATEN - 1
		If rz_zmenge(r, k) > 0
			m = Inventar_Zaehlen(rz_zutat(r, k)) / rz_zmenge(r, k)
			If m < n Then n = m
		EndIf
	Next
	Return n
End Function

; Bis zu n Auftraege fuer Rezept r anlegen: so viele, wie Zutaten und
; Warteschlange hergeben. Gibt die Zahl der angelegten zurueck.
Function Rezept_Herstellen(r, n = 1)
	If r < 0 Or r >= rz_anzahl Then Return 0
	If Not rz_frei(r) Then Return 0
	If rz_station(r) <> RZ_HAND Then Return 0
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
		rz_auftrag(rz_auftraege) = r
		rz_auftraege = rz_auftraege + 1
	Next
	Return n
End Function

; Auftrag a abbrechen: die Zutaten kommen zurueck. Passt nicht alles
; hinein, bleibt er stehen.
Function Auftrag_Abbrechen(a)
	If a < 0 Or a >= rz_auftraege Then Return False
	r = rz_auftrag(a)
	For z = 0 To RZ_ZUTATEN - 1
		If rz_zmenge(r, z) > 0
			If Not Inventar_Passt(rz_zutat(r, z), rz_zmenge(r, z))
				rz_meldung = "Kein Platz fuer die Zutaten"
				Return False
			EndIf
		EndIf
	Next
	For z = 0 To RZ_ZUTATEN - 1
		If rz_zmenge(r, z) > 0 Then Inventar_Hinzu(rz_zutat(r, z), rz_zmenge(r, z))
	Next
	For i = a To rz_auftraege - 2
		rz_auftrag(i) = rz_auftrag(i + 1)
	Next
	rz_auftraege = rz_auftraege - 1
	If a = 0 Then rz_fortschritt = 0
	Return True
End Function

; Die Zeit laufen lassen. Gibt True zurueck, wenn ein Auftrag fertig wurde.
Function Rezepte_Takt(dt#)
	If rz_auftraege = 0 Then Return False
	r = rz_auftrag(0)
	rz_fortschritt = rz_fortschritt + dt
	If rz_fortschritt < rz_zeit(r) Then Return False
	rz_fortschritt = rz_zeit(r)
	If Not Inventar_Passt(rz_ergebnis(r), rz_menge(r))
		If rz_meldung = "" Then rz_meldung = "Inventar voll - " + dg_name(rz_ergebnis(r)) + " wartet"
		Return False
	EndIf
	Inventar_Hinzu(rz_ergebnis(r), rz_menge(r))
	For i = 0 To rz_auftraege - 2
		rz_auftrag(i) = rz_auftrag(i + 1)
	Next
	rz_auftraege = rz_auftraege - 1
	rz_fortschritt = 0
	Return True
End Function

; Anteil 0..1 des vordersten Auftrags.
Function Rezepte_Anteil#()
	If rz_auftraege = 0 Then Return 0
	Return rz_fortschritt / rz_zeit(rz_auftrag(0))
End Function

; ---- Zeichnen und Klicks ---------------------------------------------------

Const RZ_BREITE = 372
Const RZ_ZEILE = 48
Const RZ_FELD = 44                  ; Abstand der Auftraege

; Die Liste steht rechts neben dem Inventar, oben buendig.
Function Rezepte_Links()
	Return Platz_X(0) - 14 + INV_LEISTE * INV_FELD + 24 + 12
End Function
Function Rezepte_Oben()
	Return Platz_Y(INV_LEISTE) - 40
End Function

; Die n-te bekannte Handarbeit (-1, wenn es so viele nicht gibt).
Function Rezept_Zeile(n)
	For r = 0 To rz_anzahl - 1
		If rz_frei(r) And rz_station(r) = RZ_HAND
			If n = 0 Then Return r
			n = n - 1
		EndIf
	Next
	Return -1
End Function

Function Rezepte_Bekannt()
	n = 0
	For r = 0 To rz_anzahl - 1
		If rz_frei(r) And rz_station(r) = RZ_HAND Then n = n + 1
	Next
	Return n
End Function

Function Rezepte_Schlange_Y()
	n = Rezepte_Bekannt() : If n = 0 Then n = 1
	Return Rezepte_Oben() + 36 + n * RZ_ZEILE + 30
End Function

; Unter dem Mauszeiger: Rezept (-1 = keins) bzw. Auftrag (-1 = keiner).
Function Rezept_Unter(mx, my)
	x = Rezepte_Links() : y0 = Rezepte_Oben() + 36
	If mx < x + 4 Or mx >= x + RZ_BREITE - 4 Or my < y0 Then Return -1
	Return Rezept_Zeile((my - y0) / RZ_ZEILE)
End Function
Function Auftrag_Unter(mx, my)
	x = Rezepte_Links() + 12 : y = Rezepte_Schlange_Y()
	If my < y Or my >= y + SYMBOL_GROESSE Or mx < x Then Return -1
	a = (mx - x) / RZ_FELD
	If a >= rz_auftraege Or (mx - x) Mod RZ_FELD >= SYMBOL_GROESSE Then Return -1
	Return a
End Function

Function Balken_Zeichnen(x, y, breite, anteil#)
	Color 0, 0, 0 : Rect x, y, breite, 5, 1
	Color 90, 200, 90 : Rect x + 1, y + 1, Int((breite - 2) * anteil), 3, 1
End Function

Function Rezepte_Zeichnen()
	x = Rezepte_Links() : y = Rezepte_Oben()
	tief = Rezepte_Schlange_Y() + SYMBOL_GROESSE + 30 - y
	Color 40, 40, 40 : Rect x, y, RZ_BREITE, tief, 1
	Color 160, 160, 160 : Rect x, y, RZ_BREITE, tief, 0
	Color 255, 255, 255
	Text x + 14, y + 12, "Herstellen"
	Color 170, 170, 170
	Text_Rechts(x + RZ_BREITE - 14, y + 12, "Klick: 1   Umschalt+Klick: alle")
	mx = MouseX() : my = MouseY()
	ueber = Rezept_Unter(mx, my)
	n = Rezepte_Bekannt()
	If n = 0
		Color 200, 200, 200
		Text x + 14, y + 44, "Noch keine Rezepte - sammle Zutaten:"
		Text x + 14, y + 62, "Aeste aus Laub, Brocken von losen Steinen"
	EndIf
	For i = 0 To n - 1
		r = Rezept_Zeile(i)
		zy = y + 36 + i * RZ_ZEILE
		moeglich = Rezept_Moeglich(r)
		If r = ueber
			If moeglich > 0 Then Color 70, 80, 70 Else Color 70, 60, 60
			Rect x + 4, zy, RZ_BREITE - 8, RZ_ZEILE - 4, 1
		EndIf
		If inv_symbol(rz_ergebnis(r)) Then DrawImage inv_symbol(rz_ergebnis(r)), x + 10, zy + 2
		titel$ = dg_name(rz_ergebnis(r))
		If rz_menge(r) > 1 Then titel = rz_menge(r) + " " + titel
		If moeglich > 0 Then Color 255, 255, 255 Else Color 150, 150, 150
		Text x + 60, zy + 5, titel
		Color 170, 170, 170
		Text_Rechts(x + RZ_BREITE - 14, zy + 5, Replace(Str(rz_zeit(r)), ".0", "") + " s")
		; Zutaten: genug gruen, zu wenig rot, mit dem Vorrat dahinter
		zx = x + 60
		For k = 0 To RZ_ZUTATEN - 1
			If rz_zmenge(r, k) > 0
				d = rz_zutat(r, k) : hab = Inventar_Zaehlen(d)
				s$ = rz_zmenge(r, k) + " " + dg_name(d) + " (" + hab + ")"
				If hab >= rz_zmenge(r, k) Then Color 140, 220, 140 Else Color 230, 120, 110
				Text zx, zy + 23, s
				zx = zx + StringWidth(s) + 14
			EndIf
		Next
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
		Balken_Zeichnen(x + 12, sy + SYMBOL_GROESSE + 6, RZ_BREITE - 24, Rezepte_Anteil())
		rest# = rz_zeit(r) - rz_fortschritt
		Color 200, 200, 200
		Text x + 14, sy + SYMBOL_GROESSE + 14, dg_name(rz_ergebnis(r)) + " - noch " + Replace(Str(Int(rest * 10) / 10.0), ".", ",") + " s"
	EndIf
End Function

; Bei geschlossenem Inventar: der laufende Auftrag rechts neben der
; Schnellleiste, mit Balken.
Function Rezepte_Leiste_Zeichnen()
	If rz_auftraege = 0 Then Return
	x = (GraphicsWidth() + INV_LEISTE * INV_FELD) / 2 + 14
	y = GraphicsHeight() - INV_FELD - 8 + 4
	e = rz_ergebnis(rz_auftrag(0))
	If inv_symbol(e) Then DrawImage inv_symbol(e), x, y - 4
	Balken_Zeichnen(x, y + SYMBOL_GROESSE - 2, SYMBOL_GROESSE, Rezepte_Anteil())
	If rz_auftraege > 1
		Color 0, 0, 0 : Text x + SYMBOL_GROESSE + 7, y + 9, "+" + (rz_auftraege - 1)
		Color 255, 255, 255 : Text x + SYMBOL_GROESSE + 6, y + 8, "+" + (rz_auftraege - 1)
	EndIf
End Function
