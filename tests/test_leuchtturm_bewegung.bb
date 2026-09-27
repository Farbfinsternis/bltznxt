; Leuchtturm Schritt 3 - Bewegung und Kollision in der Platzhalter-Arena.
;
; Bindet die Spielmodule ein und steuert den Spieler mit kuenstlicher
; Eingabe, 60 Takte je Sekunde. Die Arena erzeugt
; samples/leuchtturm/werkzeug/arena.py. Gemeldet werden die Fuesse
; (Mitte - 0.9) in Zentimetern, das Tempo in cm/s und ob Boden unter den
; Fuessen ist.

Include "../samples/leuchtturm/karte.bb"
Include "../samples/leuchtturm/spieler.bb"

Graphics3D 320,240,0,2
SetBuffer BackBuffer()

Function C$(x#)
	Return Int(Floor(x * 100.0 + 0.5))
End Function
Function Wo$(p.Spieler)
	k = p\koerper
	Return "fuesse " + C(EntityX(k)) + "," + C(EntityY(k) - SP_HALB) + "," + C(EntityZ(k)) + " tempo " + C(Spieler_Tempo(p)) + " vy " + C(p\vy) + " boden " + p\boden
End Function

Global hoechst#, luft
; `takte` Takte mit fester Eingabe; springen nur im ersten Takt. Merkt sich
; die hoechste Fusshoehe und wie viele Takte kein Boden da war.
Function Lauf(p.Spieler, takte, vor#, seit#, springen)
	hoechst = -1000 : luft = 0
	For t = 1 To takte
		Spieler_Takt(p, vor, seit, springen And t = 1)
		UpdateWorld
		Spieler_Nach(p)
		y# = EntityY(p\koerper) - SP_HALB
		If y > hoechst Then hoechst = y
		If Not p\boden Then luft = luft + 1
	Next
End Function

Karte_Laden("samples/leuchtturm/daten/arena.glb")
Print "karte: spawn " + (karte_spawn <> 0) + " col " + karte_col_zahl + " spawn bei " + C(EntityX(karte_spawn, True)) + "," + C(EntityY(karte_spawn, True)) + "," + C(EntityZ(karte_spawn, True))
p.Spieler = Spieler_Neu(EntityX(karte_spawn, True), EntityY(karte_spawn, True) + 1, EntityZ(karte_spawn, True), EntityYaw(karte_spawn, True))

; 1) Fallen: einen Meter ueber dem Spawn, eine Sekunde
Lauf(p, 60, 0, 0, False)
Print "1 fallen:   " + Wo(p)

; 2) Laufen nach vorn (+z), eine Sekunde, neben der Saeule vorbei
Spieler_Setzen(p, 4, 0, -15, 0)
Lauf(p, 10, 0, 0, False)
Lauf(p, 60, 1, 0, False)
Print "2 laufen:   " + Wo(p) + " luft " + luft
; ... und loslassen: eine halbe Sekunde
Lauf(p, 30, 0, 0, False)
Print "2 stehen:   " + Wo(p)

; 3) Gegen die Nordwand (z = 20): die Mitte bleibt 0.4 davor stehen
Spieler_Setzen(p, 0, 0, 17, 0)
Lauf(p, 90, 1, 0, False)
Print "3 wand:     " + Wo(p)

; 4) Springen aus dem Stand
Spieler_Setzen(p, 0, 0, -15, 0)
Lauf(p, 10, 0, 0, False)
Lauf(p, 60, 0, 0, True)
Print "4 sprung:   hoechstens " + C(hoechst) + " | " + Wo(p)

; 5) Aus dem Stand auf den Block (1 m hoch, x -10..-7, z -10..-7): kurz vor
;    der Kante abspringen und nur mit Luftkontrolle nach vorn druecken
Spieler_Setzen(p, -8.5, 0, -10.5, 0)
Lauf(p, 10, 0, 0, False)
Lauf(p, 25, 1, 0, True)
Lauf(p, 60, 0, 0, False)
Print "5 block:    " + Wo(p)

; 6) Die Rampe hinauf auf Plattform A (Oberkante 2): Blick nach +x
Spieler_Setzen(p, 0, 0, 11, -90)
Lauf(p, 10, 0, 0, False)
Lauf(p, 70, 1, 0, False)
Lauf(p, 60, 0, 0, False)
Print "6 rampe:    " + Wo(p)

; 7) Die Treppe hinauf auf Plattform B (6 Stufen zu 0.25): Blick nach -x
Spieler_Setzen(p, -3, 0, 11, 90)
Lauf(p, 10, 0, 0, False)
Lauf(p, 55, 1, 0, False)
Lauf(p, 60, 0, 0, False)
Print "7 treppe:   " + Wo(p)

; 7b) Rampe hinab: von Plattform A nach -x, ohne abzuheben
Spieler_Setzen(p, 12, 2, 11, 90)
Lauf(p, 10, 0, 0, False)
Lauf(p, 75, 1, 0, False)
Print "7b hinab:   " + Wo(p) + " luft " + luft

; 8) Auf dem Hang stehen: mitten auf der Rampe (bei x = 5 ist sie 1 hoch)
Spieler_Setzen(p, 5, 1.05, 11, -90)
Lauf(p, 120, 0, 0, False)
Print "8 hang:     " + Wo(p)
