; BLTZCRFT - Rezepte: Freischalten beim Aufheben, Herstellen ueber die Zeit,
; Warteschlange, Abbrechen und volles Inventar (rezepte.bb).
;
; Ein Rezept wird erst frei, wenn jede seiner Zutaten schon einmal im
; Inventar war. Herstellen zieht die Zutaten sofort ab, das Ergebnis kommt
; nach rz_zeit Sekunden; die Warteschlange fasst 8 Auftraege. Abbrechen gibt
; die Zutaten zurueck, ein fertiger Auftrag wartet, bis Platz ist.

Include "../samples/bltzcrft/bloecke.bb"
Include "../samples/bltzcrft/dinge.bb"
Include "../samples/bltzcrft/inventar.bb"
Include "../samples/bltzcrft/rezepte.bb"

Function Frei$()
	s$ = ""
	For r = 0 To rz_anzahl - 1
		If rz_frei(r) Then s = s + dg_name(rz_ergebnis(r)) + " "
	Next
	If s = "" Then s = "-"
	Return s
End Function
Function Vorrat$()
	Return "ast " + Inventar_Zaehlen(G_AST) + " brocken " + Inventar_Zaehlen(G_BROCKEN) + " axt " + Inventar_Zaehlen(G_STEINAXT) + " schaufel " + Inventar_Zaehlen(G_STEINSCHAUFEL)
End Function
Function Schlange$()
	s$ = rz_auftraege + ":"
	For a = 0 To rz_auftraege - 1
		s = s + " " + dg_name(rz_ergebnis(rz_auftrag(a)))
	Next
	Return s
End Function
Function Meldung$()
	m$ = rz_meldung : rz_meldung = ""
	If m = "" Then Return "-"
	Return m
End Function
Function Nr(ergebnis)
	For r = 0 To rz_anzahl - 1
		If rz_ergebnis(r) = ergebnis Then Return r
	Next
	Return -1
End Function

Bloecke_Arten()
Dinge_Arten()
Inventar_Leeren()
Rezepte_Laden()
Print "rezepte " + rz_anzahl + ", frei " + Frei()

; Freischalten: ein Ast allein genuegt nicht, mit Steinbrocken schon
Inventar_Hinzu(G_AST, 5)
Rezepte_Pruefen()
Print "nur ast: frei " + Frei() + "meldung " + Meldung()
Inventar_Hinzu(G_BROCKEN, 4)
Rezepte_Pruefen()
Print "ast und brocken: frei " + Frei() + "meldung " + Meldung()
Rezepte_Pruefen()
Print "nochmal pruefen: meldung " + Meldung()

; Herstellen: Zutaten sofort weg, Ergebnis nach der Zeit
axt = Nr(G_STEINAXT) : hacke = Nr(G_STEINHACKE) : schaufel = Nr(G_STEINSCHAUFEL)
Print "moeglich axt " + Rezept_Moeglich(axt) + " schaufel " + Rezept_Moeglich(schaufel)
Print "axt: " + Rezept_Herstellen(axt) + " -> " + Vorrat() + ", schlange " + Schlange()
Print "hacke: " + Rezept_Herstellen(hacke) + " meldung " + Meldung()
Print "schaufel: " + Rezept_Herstellen(schaufel) + " -> " + Vorrat() + ", schlange " + Schlange()
Print "nach 3.9 s: fertig " + Rezepte_Takt(3.9) + " anteil " + Int(Rezepte_Anteil() * 100) + "% " + Vorrat()
Print "nach 4.1 s: fertig " + Rezepte_Takt(0.2) + " " + Vorrat() + ", schlange " + Schlange()
p = 0 : While inv_ding(p) <> G_STEINAXT And p < INV_PLAETZE - 1 : p = p + 1 : Wend
Print "neue axt auf platz " + p + ", haltbarkeit " + inv_haltbar(p) + "/" + dg_haltbar(G_STEINAXT)
Rezepte_Takt(3.0)
Print "schaufel fertig: " + Vorrat() + ", schlange " + Schlange()
Print "nicht frei: " + Rezept_Herstellen(Nr(B_FACKEL)) + ", leere schlange takt " + Rezepte_Takt(1)

; Warteschlange: hoechstens 8, Umschalt = so viele wie moeglich
Inventar_Hinzu(G_AST, 40)
Inventar_Hinzu(G_BROCKEN, 20)
Print "alle schaufeln: " + Rezept_Herstellen(schaufel, RZ_SCHLANGE) + ", schlange " + rz_auftraege + ", " + Vorrat()
Print "voll: " + Rezept_Herstellen(axt) + " meldung " + Meldung()

; Abbrechen gibt die Zutaten zurueck
Rezepte_Takt(1.5)
Print "abbrechen vorn: " + Auftrag_Abbrechen(0) + " fortschritt " + rz_fortschritt + ", schlange " + rz_auftraege + ", " + Vorrat()
Print "abbrechen ungueltig: " + Auftrag_Abbrechen(7) + " " + Auftrag_Abbrechen(-1)
While rz_auftraege > 0 : Auftrag_Abbrechen(rz_auftraege - 1) : Wend
Print "alle abgebrochen: " + Vorrat()

; Volles Inventar: der fertige Auftrag wartet und kommt, sobald Platz ist
Inventar_Leeren()
Inventar_Hinzu(G_AST, 2) : Inventar_Hinzu(G_BROCKEN, 1)
Rezept_Herstellen(schaufel)
Inventar_Hinzu(B_SAND, INV_PLAETZE * 64)
Print "voll: fertig " + Rezepte_Takt(5) + " meldung " + Meldung() + ", schlange " + rz_auftraege
Inventar_Entfernen(B_SAND, 1)
Print "noch voll (ein platz halb): fertig " + Rezepte_Takt(0.1) + ", schlange " + rz_auftraege
Inventar_Entfernen(B_SAND, 63)
Print "platz frei: fertig " + Rezepte_Takt(0.1) + ", schaufel " + Inventar_Zaehlen(G_STEINSCHAUFEL) + ", schlange " + rz_auftraege

; Stamm schaltet Aeste und Bretter frei, Kohle mit Ast die Fackel
Inventar_Leeren()
Inventar_Hinzu(B_STAMM, 1) : Inventar_Hinzu(G_KOHLE, 1)
Rezepte_Pruefen()
Print "stamm und kohle: frei " + Frei() + "meldung " + Meldung()
Rezept_Herstellen(Nr(B_BRETTER))
Rezepte_Takt(2)
Print "bretter " + Inventar_Zaehlen(B_BRETTER) + ", stamm " + Inventar_Zaehlen(B_STAMM)
Print "entfernen: rest " + Inventar_Entfernen(B_BRETTER, 6) + ", passt 64 sand " + Inventar_Passt(B_SAND, 64) + ", passt 36*64+1 " + Inventar_Passt(B_SAND, INV_PLAETZE * 64 + 1)

; Werkbank: Rezept aus Brettern und Brocken, ihre Rezepte nur in der Naehe
Inventar_Leeren()
Inventar_Hinzu(B_BRETTER, 4) : Inventar_Hinzu(G_BROCKEN, 2) : Inventar_Hinzu(B_STEIN, 8)
Rezepte_Pruefen()
Print "werkbank frei " + rz_frei(Nr(B_WERKBANK)) + ", steinziegel frei " + rz_frei(Nr(B_STEINZIEGEL)) + " meldung " + Meldung()
Rezept_Herstellen(Nr(B_WERKBANK)) : Rezepte_Takt(5)
Print "werkbank " + Inventar_Zaehlen(B_WERKBANK) + ", bretter " + Inventar_Zaehlen(B_BRETTER)
rz_nah(RZ_WERKBANK) = False
Print "ziegel ohne werkbank: " + Rezept_Herstellen(Nr(B_STEINZIEGEL)) + " meldung " + Meldung()
rz_nah(RZ_WERKBANK) = True
Print "ziegel an der werkbank: " + Rezept_Herstellen(Nr(B_STEINZIEGEL)) + ", stein " + Inventar_Zaehlen(B_STEIN)
Rezepte_Takt(2)
rz_nah(RZ_WERKBANK) = False
Print "weggegangen: fertig " + Rezepte_Takt(5) + " laeuft " + Auftrag_Laeuft() + " fortschritt " + rz_fortschritt
rz_nah(RZ_WERKBANK) = True
Print "zurueck: fertig " + Rezepte_Takt(2) + ", ziegel " + Inventar_Zaehlen(B_STEINZIEGEL)

; Reparieren: halbe Zutaten (mindestens 1), das Werkzeug geht mit und
; kommt heil zurueck; volle und Nicht-Werkzeuge gehen nicht
Inventar_Leeren()
Inventar_Hinzu(G_STEINAXT, 1, 0) : Inventar_Hinzu(G_STEINHACKE, 1) : Inventar_Hinzu(G_AST, 3) : Inventar_Hinzu(G_BROCKEN, 3)
Print "reparierbar kaputte axt " + Reparierbar(0) + ", volle hacke " + Reparierbar(1) + ", ast " + Reparierbar(2) + ", leer " + Reparierbar(9)
r = Nr(G_STEINAXT)
Zeilen_Bauen()
Print "zeilen " + zl_anzahl + ", letzte ist reparatur " + (zl_art(zl_anzahl - 1) = ZL_REPARATUR) + ", titel davor " + (zl_art(zl_anzahl - 2) = ZL_TITEL Or zl_art(zl_anzahl - 2) = ZL_REZEPT)
Print "kosten axt: ast " + Reparatur_Menge(r, 0) + " brocken " + Reparatur_Menge(r, 1) + ", schaufel brocken " + Reparatur_Menge(Nr(G_STEINSCHAUFEL), 1)
rz_nah(RZ_WERKBANK) = False
Print "ohne werkbank: " + Reparieren(0) + " meldung " + Meldung()
rz_nah(RZ_WERKBANK) = True
Print "reparieren: " + Reparieren(0) + " -> axt " + Inventar_Zaehlen(G_STEINAXT) + ", " + Vorrat() + ", schlange " + Schlange()
Rezepte_Takt(2.9)
Print "nach 2.9 s: axt " + Inventar_Zaehlen(G_STEINAXT)
Rezepte_Takt(0.2)
p = 0 : While inv_ding(p) <> G_STEINAXT And p < INV_PLAETZE - 1 : p = p + 1 : Wend
Print "nach 3.1 s: " + Ding_Titel(inv_ding(p), inv_haltbar(p)) + " meldung " + Meldung()

; Abbrechen einer Reparatur: Werkzeug mit alter Haltbarkeit und Zutaten zurueck
Inventar_Leeren()
Inventar_Hinzu(G_STEINHACKE, 1, 12) : Inventar_Hinzu(G_AST, 1) : Inventar_Hinzu(G_BROCKEN, 1)
Print "hacke reparieren: " + Reparieren(0) + ", " + Vorrat()
Print "abbrechen: " + Auftrag_Abbrechen(0) + ", " + Vorrat() + ", " + Ding_Titel(inv_ding(0), inv_haltbar(0))

; Schmelzofen: an der Werkbank aus Stein, dort Eisenbarren aus Eisenerz und
; Kohle, daraus an der Werkbank Eisenwerkzeuge; Reparatur 1 Ast + 1 Barren
Inventar_Leeren()
Inventar_Hinzu(B_STEIN, 8) : Inventar_Hinzu(B_EISEN, 3) : Inventar_Hinzu(G_KOHLE, 3) : Inventar_Hinzu(G_AST, 2)
Rezepte_Pruefen()
Print "frei ofen " + rz_frei(Nr(B_OFEN)) + ", barren " + rz_frei(Nr(G_EISENBARREN)) + ", eisenhacke " + rz_frei(Nr(G_EISENHACKE)) + " meldung " + Meldung()
rz_nah(RZ_WERKBANK) = True : rz_nah(RZ_OFEN) = False
Print "ofen bauen: " + Rezept_Herstellen(Nr(B_OFEN)) + ", stein " + Inventar_Zaehlen(B_STEIN)
Rezepte_Takt(6)
Print "ofen " + Inventar_Zaehlen(B_OFEN) + ", barren ohne ofen: " + Rezept_Herstellen(Nr(G_EISENBARREN)) + " meldung " + Meldung()
rz_nah(RZ_OFEN) = True
Print "barren am ofen: " + Rezept_Herstellen(Nr(G_EISENBARREN), RZ_SCHLANGE) + ", erz " + Inventar_Zaehlen(B_EISEN) + " kohle " + Inventar_Zaehlen(G_KOHLE)
Zeilen_Bauen()
s$ = ""
For z = 0 To zl_anzahl - 1
	If zl_art(z) = ZL_TITEL Then s = s + "[" + Station_Titel(zl_wert(z)) + "] "
Next
Print "abschnitte: " + s
rz_nah(RZ_OFEN) = False
Print "ofen weg: laeuft " + Auftrag_Laeuft() + ", fertig " + Rezepte_Takt(10)
rz_nah(RZ_OFEN) = True
Rezepte_Takt(6) : Rezepte_Takt(6) : Rezepte_Takt(6)
Rezepte_Pruefen()
Print "barren " + Inventar_Zaehlen(G_EISENBARREN) + ", eisenhacke frei " + rz_frei(Nr(G_EISENHACKE)) + " meldung " + Meldung()
Print "eisenhacke: " + Rezept_Herstellen(Nr(G_EISENHACKE))
Rezepte_Takt(6)
p = 0 : While inv_ding(p) <> G_EISENHACKE And p < INV_PLAETZE - 1 : p = p + 1 : Wend
Print Ding_Titel(inv_ding(p), inv_haltbar(p)) + ", kosten reparatur ast " + Reparatur_Menge(Nr(G_EISENHACKE), 0) + " barren " + Reparatur_Menge(Nr(G_EISENHACKE), 1)
End
