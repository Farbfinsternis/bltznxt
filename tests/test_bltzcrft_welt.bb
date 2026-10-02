; BLTZCRFT - Weltgenerierung: dieselbe Saat ergibt dieselbe Welt, und die
; Bloecke liegen logisch. Geprueft werden zwei Gebiete zu 6 x 6 Chunks: Land
; um den Ursprung und eine Kueste mit Ozean (Saat 2026).
;
; Gemeldet werden Pruefsummen zweier Chunks, wie oft jede Blockart vorkommt
; und die Zahl der Verstoesse gegen die Regeln (alle sollen 0 sein):
;   - ganz unten (y = 0) liegt Grundgestein,
;   - kein Wasser schwebt ueber Luft,
;   - neben Wasser liegt keine Luft (es liefe aus - Wasser fliesst nicht),
;   - auf Gras und Schnee liegt kein fester Block ausser Laub,
;   - ein Stamm steht auf Erde oder Stamm,
;   - eine Wasseroberflaeche liegt auf Meereshoehe (Ozean, Seen) oder
;     mindestens drei Bloecke darueber (Tuempel).

Include "../samples/bltzcrft/rauschen.bb"
Include "../samples/bltzcrft/bloecke.bb"
Include "../samples/bltzcrft/welt.bb"

Function Summe(c.Chunk)
	s = 0
	For i = 0 To CH_BYTES - 1
		s = (s * 31 + PeekByte(c\daten, i)) And $FFFFFF
	Next
	Return s
End Function

Dim anzahl(B_ANZAHL - 1)
Global v_grund, v_schwebt, v_leck, v_bedeckt, v_stamm, v_ueber_meer, v_tuempel, v_ozean

; Gebiet um Chunk (mx, mz) erzeugen und pruefen; die aeusseren Chunks sind
; nur Nachbarn, damit die Pruefung am Rand nicht ins Leere greift.
Function Gebiet(mx, mz)
	For cz = mz - 4 To mz + 3
		For cx = mx - 4 To mx + 3
			Chunk_Neu(cx, cz)
		Next
	Next
	For z = (mz - 3) * CH To (mz + 3) * CH - 1
		For x = (mx - 3) * CH To (mx + 3) * CH - 1
			If Welt_Block(x, 0, z) <> B_GRUND Then v_grund = v_grund + 1
			For y = 0 To HOEHE - 1
				b = Welt_Block(x, y, z)
				anzahl(b) = anzahl(b) + 1
				Select b
				Case B_WASSER
					If Welt_Block(x, y - 1, z) = B_LUFT Then v_schwebt = v_schwebt + 1
					If Welt_Block(x + 1, y, z) = B_LUFT Then v_leck = v_leck + 1
					If Welt_Block(x - 1, y, z) = B_LUFT Then v_leck = v_leck + 1
					If Welt_Block(x, y, z + 1) = B_LUFT Then v_leck = v_leck + 1
					If Welt_Block(x, y, z - 1) = B_LUFT Then v_leck = v_leck + 1
					If y > MEER
						v_tuempel = v_tuempel + 1
						; eine Wasseroberflaeche liegt auf Meereshoehe oder ist ein Tuempel
						If Welt_Block(x, y + 1, z) = B_LUFT And y < MEER + 3 Then v_ueber_meer = v_ueber_meer + 1
					ElseIf y = MEER
						v_ozean = v_ozean + 1
					EndIf
				Case B_GRAS, B_SCHNEE
					o = Welt_Block(x, y + 1, z)
					If bl_fest(o) And o <> B_LAUB Then v_bedeckt = v_bedeckt + 1
				Case B_STAMM
					u = Welt_Block(x, y - 1, z)
					If u <> B_ERDE And u <> B_STAMM Then v_stamm = v_stamm + 1
				End Select
			Next
		Next
	Next
End Function

Bloecke_Arten()
Welt_Saat(2026)

Gebiet(0, 0)
Print "summe 0,0: " + Summe(Chunk_Holen(0, 0))
Print "summe 2,-2: " + Summe(Chunk_Holen(2, -2))

; Neu erzeugt - auch nach anderen Chunks - muss dasselbe herauskommen
a = Summe(Chunk_Holen(1, 1))
Gebiet(-9, 13)
Chunk_Weg(Chunk_Holen(1, 1))
Chunk_Neu(1, 1)
Print "wiederholbar: " + (a = Summe(Chunk_Holen(1, 1)))

Print "bloecke:"
For b = 0 To B_ANZAHL - 1
	Print "  " + bl_name(b) + " " + anzahl(b)
Next
Print "verstoesse: grund " + v_grund + " schwebt " + v_schwebt + " leck " + v_leck + " bedeckt " + v_bedeckt + " stamm " + v_stamm + " ueber_meer " + v_ueber_meer
Print "ozean vorhanden: " + (v_ozean > 1000) + "   tuempel vorhanden: " + (v_tuempel > 0)
End
