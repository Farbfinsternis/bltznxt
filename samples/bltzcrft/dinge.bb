; BLTZCRFT - Gegenstaende
;
; Im Inventar liegen "Dinge". Die Nummern 0 bis B_ANZAHL - 1 sind die
; Bloecke (bloecke.bb), ab G_ERSTER kommen Gegenstaende, die man nicht
; setzen kann: Ast, Steinbrocken, Werkzeuge. Jedes Ding hat einen Namen,
; eine Stapelgroesse und eine Hoechsthaltbarkeit (0 = verschleisst nicht).
; Werkzeuge stapeln nicht; ihre Haltbarkeit steht je Inventarplatz
; (inventar.bb). Ist sie aufgebraucht, ist das Werkzeug kaputt, bleibt aber
; liegen - an der Werkbank laesst es sich reparieren (CRAFTING.md).
;
; Gegenstaende haben eine flache Textur wie die Pflanzen (bloecke.bb malt
; sie mit, ab T_AST); daraus entstehen Symbol und Hand.
;
; dg_gesehen merkt sich, was man schon einmal aufgehoben hat - daraus
; werden spaeter die Rezepte frei.

Const G_ERSTER = 64
Const G_AST = 64
Const G_BROCKEN = 65
Const G_STEINAXT = 66
Const G_STEINHACKE = 67
Const G_STEINSCHAUFEL = 68
Const G_KOHLE = 69
Const G_EISENBARREN = 70
Const G_EISENAXT = 71
Const G_EISENHACKE = 72
Const G_EISENSCHAUFEL = 73
Const D_ANZAHL = 74

Const STEIN_HALTBAR = 100       ; Haltbarkeit der Steinwerkzeuge
Const EISEN_HALTBAR = 250       ; ... der Eisenwerkzeuge (wie in Minecraft)

Dim dg_name$(D_ANZAHL - 1)
Dim dg_tex(D_ANZAHL - 1)        ; flache Textur, -1 = Block (eigenes Symbol)
Dim dg_stapel(D_ANZAHL - 1)
Dim dg_haltbar(D_ANZAHL - 1)    ; Hoechsthaltbarkeit, 0 = keine
Dim dg_werkzeug(D_ANZAHL - 1)   ; W_...
Dim dg_stufe(D_ANZAHL - 1)      ; 0 Hand, 1 Stein, 2 Eisen
Dim dg_gesehen(D_ANZAHL - 1)

Function Ding_Art(nr, name$, t, stapel, haltbar, werkzeug, stufe)
	dg_name(nr) = name
	dg_tex(nr) = t
	dg_stapel(nr) = stapel
	dg_haltbar(nr) = haltbar
	dg_werkzeug(nr) = werkzeug
	dg_stufe(nr) = stufe
End Function

; Nach Bloecke_Arten: die Bloecke uebernehmen, dann die Gegenstaende.
Function Dinge_Arten()
	For b = 0 To D_ANZAHL - 1
		If b < B_ANZAHL Then Ding_Art(b, bl_name(b), -1, 64, 0, W_KEINS, 0) Else Ding_Art(b, "", -1, 0, 0, W_KEINS, 0)
		dg_gesehen(b) = False
	Next
	Ding_Art(G_AST, "Ast", T_AST, 64, 0, W_KEINS, 0)
	Ding_Art(G_BROCKEN, "Steinbrocken", T_BROCKEN, 64, 0, W_KEINS, 0)
	Ding_Art(G_STEINAXT, "Steinaxt", T_STEINAXT, 1, STEIN_HALTBAR, W_AXT, 1)
	Ding_Art(G_STEINHACKE, "Steinspitzhacke", T_STEINHACKE, 1, STEIN_HALTBAR, W_HACKE, 1)
	Ding_Art(G_STEINSCHAUFEL, "Steinschaufel", T_STEINSCHAUFEL, 1, STEIN_HALTBAR, W_SCHAUFEL, 1)
	Ding_Art(G_KOHLE, "Kohle", T_KOHLESTUECK, 64, 0, W_KEINS, 0)
	Ding_Art(G_EISENBARREN, "Eisenbarren", T_EISENBARREN, 64, 0, W_KEINS, 0)
	Ding_Art(G_EISENAXT, "Eisenaxt", T_EISENAXT, 1, EISEN_HALTBAR, W_AXT, 2)
	Ding_Art(G_EISENHACKE, "Eisenspitzhacke", T_EISENHACKE, 1, EISEN_HALTBAR, W_HACKE, 2)
	Ding_Art(G_EISENSCHAUFEL, "Eisenschaufel", T_EISENSCHAUFEL, 1, EISEN_HALTBAR, W_SCHAUFEL, 2)
End Function

Function Ist_Block(d)
	Return d >= 0 And d < B_ANZAHL
End Function

; Die flache Textur, mit der ein Ding in Symbol und Hand erscheint; -1 fuer
; Bloecke, die als Wuerfel gezeigt werden.
Function Ding_Flach(d)
	If Ist_Block(d)
		If bl_pflanze(d) Or bl_fackel(d) Then Return bl_tex(d, 1)
		Return -1
	EndIf
	Return dg_tex(d)
End Function

; Name mit Zustand, etwa "Steinaxt (37/100)" oder "Steinaxt (kaputt)".
Function Ding_Titel$(d, h)
	If dg_haltbar(d) = 0 Then Return dg_name(d)
	If h <= 0 Then Return dg_name(d) + " (kaputt)"
	Return dg_name(d) + " (" + h + "/" + dg_haltbar(d) + ")"
End Function

; ---- Werkzeuge beim Abbauen ------------------------------------------------
;
; Wie in Minecraft: Ein Werkzeug wirkt auf einen Block, wenn seine Art zum
; Block passt (bl_werkzeug) und es nicht kaputt ist. Dann bricht der Block
; schneller (Steinstufe 4-mal so schnell). Ertrag gibt es, wo die Hand
; genuegt (bl_stufe 0) oder ein wirkendes Werkzeug mindestens die verlangte
; Stufe hat. d ist das gehaltene Ding, h seine Haltbarkeit.

Function Werkzeug_Wirkt(b, d, h)
	If h <= 0 Or dg_werkzeug(d) = W_KEINS Then Return False
	Return dg_werkzeug(d) = bl_werkzeug(b)
End Function

Function Kann_Ernten(b, d, h)
	If bl_stufe(b) = 0 Then Return True
	If Not Werkzeug_Wirkt(b, d, h) Then Return False
	Return dg_stufe(d) >= bl_stufe(b)
End Function

; Wie viel schneller als die Hand.
Function Werkzeug_Tempo#(b, d, h)
	If Not Werkzeug_Wirkt(b, d, h) Then Return 1
	Select dg_stufe(d)
	Case 1 : Return 4
	Case 2 : Return 6
	End Select
	Return 1
End Function

Function Werkzeug_Name$(w)
	Select w
	Case W_HACKE : Return "Spitzhacke"
	Case W_AXT : Return "Axt"
	Case W_SCHAUFEL : Return "Schaufel"
	End Select
	Return ""
End Function

; Warum von Block b nichts uebrig blieb - "" wenn doch.
Function Ertrag_Hinweis$(b, d, h)
	If Kann_Ernten(b, d, h) Then Return ""
	w = bl_werkzeug(b)
	If dg_werkzeug(d) = w And w <> W_KEINS
		If h <= 0 Then Return dg_name(d) + " ist kaputt - so bleibt von " + bl_name(b) + " nichts uebrig"
		Return "Fuer " + bl_name(b) + " reicht eine " + dg_name(d) + " nicht"
	EndIf
	If d = B_LUFT Then Return "Mit blosser Hand bleibt von " + bl_name(b) + " nichts uebrig - dafuer braucht es eine " + Werkzeug_Name(w)
	Return "Ohne " + Werkzeug_Name(w) + " bleibt von " + bl_name(b) + " nichts uebrig"
End Function
