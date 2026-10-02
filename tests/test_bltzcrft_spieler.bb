; BLTZCRFT - Spieler: Bewegung und Kollision gegen das Blockraster.
;
; Die Chunks werden erzeugt und dann von Hand ueberschrieben: ein Boden aus
; Stein (oberster Block y = 9), eine Wand bei x = 12, eine einen Block hohe
; Stufe (x 2..5, z 6..9) und ein Becken mit Wasser (x -10..-6, z -10..-6,
; Wasser y 6..9). Gesteuert wird mit kuenstlicher Eingabe, 60 Takte je
; Sekunde. Gemeldet werden die Fuesse in Zentimetern.

Const SICHT = 2
Include "../samples/bltzcrft/rauschen.bb"
Include "../samples/bltzcrft/bloecke.bb"
Include "../samples/bltzcrft/welt.bb"
Include "../samples/bltzcrft/spieler.bb"

Graphics3D 320, 240, 0, 2
SetBuffer BackBuffer()

Function C$(x#)
	Return Int(Floor(x * 100.0 + 0.5))
End Function
Function Wo$()
	Return "fuesse " + C(sp_x) + "," + C(sp_y) + "," + C(sp_z) + " boden " + sp_boden + " wasser " + sp_wasser
End Function

Global hoechst#
Function Lauf(takte, vor#, seit#, hoch)
	hoechst = -1000
	For t = 1 To takte
		Spieler_Takt(vor, seit, hoch, False, False)
		If sp_y > hoechst Then hoechst = sp_y
	Next
End Function

Bloecke_Arten()
Welt_Saat(1)
For cz = -1 To 1
	For cx = -1 To 1
		c.Chunk = Chunk_Neu(cx, cz)
		For i = 0 To CH_BYTES - 1 : PokeByte c\daten, i, B_LUFT : Next
		For z = 0 To 15
			For x = 0 To 15
				PokeByte c\daten, Ofs(x, 0, z), B_GRUND
				For y = 1 To 9 : PokeByte c\daten, Ofs(x, y, z), B_STEIN : Next
			Next
		Next
	Next
Next
For z = -16 To 31
	For y = 10 To 13 : Welt_Setzen(12, y, z, B_STEIN) : Next
Next
For z = 6 To 9
	For x = 2 To 5 : Welt_Setzen(x, 10, z, B_STEIN) : Next
Next
For z = -10 To -6
	For x = -10 To -6
		For y = 6 To 9 : Welt_Setzen(x, y, z, B_WASSER) : Next
	Next
Next

Spieler_Neu(0.5, 20, 0.5)

Lauf(120, 0, 0, False)
Print "1 fallen:  " + Wo()

sp_gier = 0
Lauf(60, 1, 0, False)
Print "2 laufen:  " + Wo() + " tempo " + C(Sqr(sp_vx * sp_vx + sp_vz * sp_vz))
Lauf(60, 0, 0, False)
Print "2 stehen:  " + Wo()

Spieler_Setzen(6.5, 10, 0.5) : sp_gier = -90
Lauf(120, 1, 0, False)
Print "3 wand:    " + Wo()

Spieler_Setzen(0.5, 10, 0.5)
Lauf(1, 0, 0, False)
Lauf(1, 0, 0, True) : h1# = hoechst
Lauf(59, 0, 0, False)
If h1 > hoechst Then hoechst = h1
Print "4 sprung:  hoechstens " + C(hoechst - 10) + " | " + Wo()

Spieler_Setzen(3.5, 10, 2.5) : sp_gier = 0
Lauf(60, 1, 0, False)
Print "5 stufe:   " + Wo()
Lauf(60, 1, 0, True)
Print "5 hinauf:  " + Wo()

Spieler_Setzen(-8, 10, -3) : sp_gier = 180
Lauf(40, 1, 0, False)
Lauf(180, 0, 0, False)
Print "6 sinken:  " + Wo()
Lauf(120, 0, 0, True)
Print "6 auftauchen: " + (sp_y > 8.3) + " | wasser " + sp_wasser

Spieler_Setzen(0.5, 10, 0.5)
sp_fliegen = True
Lauf(30, 0, 0, True)
h1 = sp_y
Lauf(30, 0, 0, False)
Print "7 fliegen: steigt " + (h1 > 13) + " schwebt danach " + (Abs(sp_vy) < 0.5 And sp_y > 13)
sp_fliegen = False

Spieler_Setzen(0.5, 10, 0.5) : sp_gier = 0 : sp_neig = 90
Spieler_Kamera(1)
Print "8 blick:   treffer " + Blick_Strahl() + " block " + ziel_x + "," + ziel_y + "," + ziel_z + " davor " + ziel_vx + "," + ziel_vy + "," + ziel_vz
Print "8 platz:   unter mir frei " + Platz_Frei(0, 10, 0) + ", daneben frei " + Platz_Frei(3, 10, 0)
sp_neig = 0 : sp_gier = -90
Spieler_Setzen(9.5, 10, 0.5)
Spieler_Kamera(1)
Print "8 wand:    treffer " + Blick_Strahl() + " block " + ziel_x + "," + ziel_y + "," + ziel_z + " davor " + ziel_vx + "," + ziel_vy + "," + ziel_vz
End
