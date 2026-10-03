; BLTZCRFT - Inventar: Stapeln, Nehmen, die Klicks im offenen Inventar und
; die zur Laufzeit gemalten Blocksymbole.
;
; Die Symbole liest das Spiel mit ReadPixelFast aus den Blocktexturen
; (TextureBuffer) und schreibt sie mit WritePixelFast in ein Bild
; (ImageBuffer). Geprueft wird ein Texel des Deckels und je einer der beiden
; Seiten, abgedunkelt wie im Spiel (256, 205 und 150 von 256), und dass die
; Ecken frei bleiben (Maskenfarbe Schwarz).

Include "../samples/bltzcrft/bloecke.bb"
Include "../samples/bltzcrft/inventar.bb"

Graphics3D 320, 240, 0, 2
SetBuffer BackBuffer()

Function Zeile$(von, bis)
	s$ = ""
	For i = von To bis
		If inv_anzahl(i) > 0 Then s = s + bl_name(inv_block(i)) + "x" + inv_anzahl(i) + " " Else s = s + "- "
	Next
	Return s
End Function
Function Hand$()
	If inv_hand_anzahl = 0 Then Return "leer"
	Return bl_name(inv_hand_block) + "x" + inv_hand_anzahl
End Function

Bloecke_Laden()
Inventar_Leeren()

Print "ertrag gras " + bl_name(Block_Ertrag(B_GRAS)) + ", stein " + bl_name(Block_Ertrag(B_STEIN)) + ", wasser " + bl_name(Block_Ertrag(B_WASSER)) + ", grund " + bl_name(Block_Ertrag(B_GRUND))

; Stapeln: erst auffuellen, dann neue Plaetze - Leiste zuerst
Inventar_Hinzu(B_STEIN, 1)
Inventar_Hinzu(B_ERDE, 3)
Inventar_Hinzu(B_STEIN, 70)
Print "stapeln " + Zeile(0, 4)
Print "gewaehlt " + bl_name(Inventar_Gewaehlt())

; Nehmen bis leer
inv_gewaehlt = 1
For i = 1 To 3 : Inventar_Nehmen(1) : Next
Print "nach 3x nehmen " + Zeile(0, 4) + " gewaehlt " + bl_name(Inventar_Gewaehlt())
Print "leer nehmen " + Inventar_Nehmen(1)

; Voll: 36 Stapel Sand passen, der Rest kommt zurueck
Inventar_Leeren()
rest = Inventar_Hinzu(B_SAND, 36 * 64 + 5)
Print "voll rest " + rest + " letzter " + Zeile(35, 35)
Inventar_Leeren()

; Klicks
Inventar_Hinzu(B_KIES, 10)
Inventar_Hinzu(B_SAND, 64)
Inventar_Hinzu(B_SAND, 20)
Print "start " + Zeile(0, 3)
Inventar_Links(0)
Print "links nimmt " + Zeile(0, 3) + "hand " + Hand()
Inventar_Links(4)
Print "links legt ab " + Zeile(0, 4) + "hand " + Hand()
Inventar_Rechts(2)
Print "rechts halbiert " + Zeile(0, 4) + "hand " + Hand()
Inventar_Rechts(5) : Inventar_Rechts(5)
Print "rechts legt ein stueck " + Zeile(0, 5) + "hand " + Hand()
Inventar_Links(1)
Print "links fuellt auf " + Zeile(0, 5) + "hand " + Hand()
Inventar_Links(4)
Print "links tauscht " + Zeile(0, 5) + "hand " + Hand()
Inventar_Schliessen()
Print "schliessen " + Zeile(0, 5) + "hand " + Hand()

; Umschalt+Klick: Leiste <-> Vorrat
Inventar_Schieben(1)
Print "schieben in vorrat " + Zeile(0, 2) + "| " + Zeile(9, 10)
Inventar_Schieben(9)
Print "schieben zurueck " + Zeile(0, 2) + "| " + Zeile(9, 10)

; Symbole
Inventar_Symbole()
n = 0
For b = 1 To B_ANZAHL - 1
	If inv_symbol(b) Then n = n + 1
Next
Print "symbole " + n + " fuer " + (B_ANZAHL - 1) + " blockarten, groesse " + ImageWidth(inv_symbol(B_STEIN)) + "x" + ImageHeight(inv_symbol(B_STEIN))

Function Texel(t, tx, ty)
	LockBuffer TextureBuffer(tx_bild(t))
	c = ReadPixelFast(tx * TEX_ZOOM + TEX_ZOOM / 2, ty * TEX_ZOOM + TEX_ZOOM / 2, TextureBuffer(tx_bild(t))) And $FFFFFF
	UnlockBuffer TextureBuffer(tx_bild(t))
	Return c
End Function
Function Dunkel(c, hell)
	r = ((c Shr 16) And 255) * hell / 256 : g = ((c Shr 8) And 255) * hell / 256 : b = (c And 255) * hell / 256
	Return (r Shl 16) Or (g Shl 8) Or b
End Function
Function Bildpunkt(img, x, y)
	LockBuffer ImageBuffer(img)
	c = ReadPixelFast(x, y, ImageBuffer(img)) And $FFFFFF
	UnlockBuffer ImageBuffer(img)
	Return c
End Function

; Stamm: Deckel = Jahresringe, Seiten = Rinde. Bei 40 Bildpunkten (h = 20,
; q = 10, Bildpunktmitte +0.5) liegt (20,1) im Deckel bei u,v = 0.0875,0.0625
; (Texel 1,1); (2,20) links bei u,v = 0.125,0.4625 (Texel 2,7); (37,20)
; rechts bei u,v = 0.875,0.4625 (Texel 14,7).
img = inv_symbol(B_STAMM)
oben_ok = (Bildpunkt(img, 20, 1) = Texel(T_STAMM_OBEN, 1, 1))
links_ok = (Bildpunkt(img, 2, 20) = Dunkel(Texel(T_STAMM_SEITE, 2, 7), 205))
rechts_ok = (Bildpunkt(img, 37, 20) = Dunkel(Texel(T_STAMM_SEITE, 14, 7), 150))
ecke_frei = (Bildpunkt(img, 0, 0) = 0 And Bildpunkt(img, 39, 0) = 0)
Print "symbol stamm: deckel " + oben_ok + " links " + links_ok + " rechts " + rechts_ok + " ecken frei " + ecke_frei

; Gezeichnet: die freien Ecken lassen den Hintergrund durch
Cls
Color 0, 0, 255 : Rect 0, 0, 60, 60, 1
DrawImage img, 10, 10
LockBuffer BackBuffer()
hinten = ReadPixelFast(10, 10) And $FFFFFF : vorn = ReadPixelFast(30, 31) And $FFFFFF
UnlockBuffer BackBuffer()
Print "gezeichnet ecke " + Right(Hex(hinten), 6) + " mitte nicht blau " + (vorn <> $0000FF)
End
