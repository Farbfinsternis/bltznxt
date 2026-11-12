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
End
