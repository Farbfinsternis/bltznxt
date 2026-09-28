; Leuchtturm - Items (Schritt 7).
;
; An den Marken der Karte liegen Waffen, Munition, Ruestung und Gesundheit.
; Der Name sagt, was (LEUCHTTURM.md, "Die Karte in Blender"):
;
;   weapon_mg, weapon_rl, weapon_rail   die Waffe und Munition (40, 10, 10)
;   ammo_mg, ammo_rl, ammo_rail         Munition (50, 5, 10)
;   armor_N                             N Punkte Ruestung, hoechstens 200
;   health_N                            N Leben, bis 100 - ab 100 Punkten
;                                       ("Mega") bis 200
;
; Die Mengen und Zeiten sind die von Quake III: Waffen kommen nach 5 s
; wieder, Ruestung nach 25, Gesundheit nach 35, Munition nach 40. Ein Item
; wird nur genommen, wenn es etwas bringt - wer 100 Leben hat, laesst die
; kleine Gesundheit liegen.
;
; Beruehrt ist ein Item wie in Quake, wenn sich die Kaesten ueberlappen:
; der des Spielers (0.8 x 1.8 x 0.8) und einer von einem knappen Meter um
; das Item (Quake: 30 Einheiten).
;
; Die Modelle: Waffen sind ihre eigenen Modelle (daten/mg.glb ...), gross
; genug zum Sehen; der Rest liegt in daten/items/ (werkzeug/kenney.py). Fehlt
; die Datei zu armor_75, nimmt das Spiel die naechstkleinere, die es gibt.
;
;   Items_Laden ordner$, waffen_ordner$
;   Items_Takt p                ; je Takt

Const IT_WAFFE = 1, IT_MUNITION = 2, IT_RUESTUNG = 3, IT_GESUNDHEIT = 4
Const IT_HALB# = 0.48           ; halbe Kantenlaenge des Kastens um ein Item

Global it_meldung$, it_meldung_zeit#

Type Item
	Field ent
	Field art, nr, menge
	Field x#, y#, z#
	Field weg               ; Takte, bis es wieder da ist; 0 = liegt da
	Field wieder#           ; so viele Sekunden nach dem Aufsammeln
	Field phase#
End Type

; Geladene Modelle, jedes nur einmal
Type ItemVorlage
	Field datei$, ent
End Type

Function Items_Laden(ordner$, waffen_ordner$)
	Delete Each Item
	n = 0
	For m.Marke = Each Marke
		it.Item = Null
		If Left(m\name, 7) = "weapon_"
			nr = Item_Waffe(Mid(m\name, 8))
			If nr
				it = Item_Neu(m, IT_WAFFE, nr, 0, 5)
				Select nr
				Case W_MG : it\menge = 40
				Default : it\menge = 10
				End Select
				it\ent = Item_Modell(waffen_ordner + "/" + Mid(m\name, 8) + ".glb", 2.5)
			EndIf
		ElseIf Left(m\name, 5) = "ammo_"
			nr = Item_Waffe(Mid(m\name, 6))
			If nr
				it = Item_Neu(m, IT_MUNITION, nr, 0, 40)
				Select nr
				Case W_MG : it\menge = 50
				Case W_RL : it\menge = 5
				Default : it\menge = 10
				End Select
				it\ent = Item_Modell(ordner + "/" + m\name + ".glb", 1)
			EndIf
		ElseIf Left(m\name, 6) = "armor_"
			it = Item_Neu(m, IT_RUESTUNG, 0, Int(Mid(m\name, 7)), 25)
			it\ent = Item_Modell(Item_Datei(ordner, "armor_", it\menge), 1)
		ElseIf Left(m\name, 7) = "health_"
			it = Item_Neu(m, IT_GESUNDHEIT, 0, Int(Mid(m\name, 8)), 35)
			it\ent = Item_Modell(Item_Datei(ordner, "health_", it\menge), 1)
		EndIf
		If it <> Null
			it\phase = n * 0.37
			n = n + 1
			PositionEntity it\ent, it\x, it\y, it\z
			EntityFX it\ent, 1 + 2          ; hell, damit man es auch im Schatten sieht
		EndIf
	Next
End Function

Function Item_Neu.Item(m.Marke, art, nr, menge, wieder#)
	it.Item = New Item
	it\art = art : it\nr = nr : it\menge = menge
	it\x = m\x : it\y = m\y : it\z = m\z
	it\wieder = wieder
	Return it
End Function

Function Item_Waffe(n$)
	Select n
	Case "mg" : Return W_MG
	Case "rl" : Return W_RL
	Case "rail" : Return W_RAIL
	End Select
	Return 0
End Function

; Die Datei zu armor_N / health_N: N selbst, sonst die naechstkleinere von 100, 50, 25.
Function Item_Datei$(ordner$, art$, menge)
	d$ = ordner + "/" + art + menge + ".glb"
	If FileType(d) = 1 Then Return d
	If menge >= 100 Then Return ordner + "/" + art + "100.glb"
	If menge >= 50 Then Return ordner + "/" + art + "50.glb"
	Return ordner + "/" + art + "25.glb"
End Function

Function Item_Modell(datei$, groesse#)
	For v.ItemVorlage = Each ItemVorlage
		If v\datei = datei Then Exit
	Next
	If v = Null
		v = New ItemVorlage
		v\datei = datei
		v\ent = LoadMesh(datei)
		If v\ent = 0 Then RuntimeError "Item nicht lesbar: " + datei
		HideEntity v\ent
	EndIf
	e = CopyEntity(v\ent)
	ShowEntity e
	ScaleEntity e, groesse, groesse, groesse
	Return e
End Function

Function Items_Takt(p.Spieler)
	If it_meldung_zeit > 0 Then it_meldung_zeit = it_meldung_zeit - TAKT
	For it.Item = Each Item
		it\phase = it\phase + TAKT
		If it\weg > 0
			it\weg = it\weg - 1
			If it\weg = 0
				ShowEntity it\ent
				Klang(kl_wieder, it\ent)
				Effekt_Sprite(it\x, it\y, it\z, fx_glanz, 3, 200, 230, 255, 1.2, 0.3, 0, 1, 0.3)
			EndIf
		Else
			PositionEntity it\ent, it\x, it\y + 0.1 * Sin(it\phase * 150), it\z
			RotateEntity it\ent, 0, it\phase * 90, 0
			If (Not p\tot) And Item_Beruehrt(p, it)
				If Item_Nehmen(p, it)
					it\weg = Int(it\wieder * 60)
					HideEntity it\ent
				EndIf
			EndIf
		EndIf
	Next
End Function

Function Item_Beruehrt(p.Spieler, it.Item)
	k = p\koerper
	If Abs(EntityX(k) - it\x) > SP_RADIUS + IT_HALB Then Return False
	If Abs(EntityZ(k) - it\z) > SP_RADIUS + IT_HALB Then Return False
	If Abs(EntityY(k) - it\y) > SP_HALB + IT_HALB Then Return False
	Return True
End Function

; Wirkung des Items; False, wenn es nichts bringt und liegen bleibt.
Function Item_Nehmen(p.Spieler, it.Item)
	Select it\art
	Case IT_WAFFE
		Waffe_Nehmen(it\nr, it\menge)
		Item_Melden(w_name(it\nr), kl_waffe, p)
	Case IT_MUNITION
		If Not Munition_Nehmen(it\nr, it\menge) Then Return False
		Item_Melden("Munition " + w_name(it\nr), kl_nehmen, p)
	Case IT_RUESTUNG
		If p\panzer >= 200 Then Return False
		p\panzer = p\panzer + it\menge
		If p\panzer > 200 Then p\panzer = 200
		Item_Melden("Ruestung +" + it\menge, kl_ruestung, p)
	Case IT_GESUNDHEIT
		grenze = 100
		If it\menge >= 100 Then grenze = 200
		If p\leben >= grenze Then Return False
		p\leben = p\leben + it\menge
		If p\leben > grenze Then p\leben = grenze
		If it\menge >= 100
			Item_Melden("Gesundheit +" + it\menge, kl_mega, p)
		Else
			Item_Melden("Gesundheit +" + it\menge, kl_gesundheit, p)
		EndIf
	End Select
	Return True
End Function

Function Item_Melden(text$, snd, p.Spieler)
	it_meldung = text
	it_meldung_zeit = 2
	Klang(snd, p\kamera)
End Function
