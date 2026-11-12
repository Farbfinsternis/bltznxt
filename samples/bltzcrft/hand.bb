; BLTZCRFT - die eigene Hand in der Ego-Sicht
;
; Rechts unten im Bild der Arm, oder - ist in der Schnellleiste ein Block
; gewaehlt - dieser Block, wie in Minecraft. Beides haengt an einem
; Drehpunkt vor der Kamera. Beim Abbauen schlaegt die Hand immer wieder zu,
; beim Setzen einmal; beim Laufen wippt sie, beim Wechsel des Blocks taucht
; sie kurz ab.
;
; Haelt man eine Fackel, leuchtet sie auch: unsichtbar steht eine Fackel am
; Auge (Handlicht), deren Licht sich wie das einer gesetzten ausbreitet.
;
; Arm und Block haben EntityOrder -1: sie werden zuletzt und ohne Z-Puffer
; gezeichnet und stecken so nie in einer Wand, vor der man steht. Weil beide
; Quader konvex sind und die Rueckseiten wegfallen, stimmt ihre Zeichnung
; auch ohne Z-Puffer. Beleuchtet sind sie wie die Welt: Vertexfarben mal
; Umgebungslicht, nachts also dunkel.

Const SCHLAG_DAUER# = 0.3          ; Sekunden je Schlag (6 Takte wie in Minecraft)
; Ruhelage des Drehpunkts vor dem Auge: rechts, unten, vorn
Const HD_X# = 0.42
Const HD_Y# = -0.40
Const HD_Z# = 0.62

Global hd_halter, hd_arm, hd_zeigen = True
Global hd_block = -1            ; der gerade gezeigte Block, B_LUFT = Arm
Global hd_schlag# = -1          ; 0..1 waehrend eines Schlags, sonst -1
Global hd_wechsel#              ; 1 nach einem Wechsel, faellt auf 0
Global hd_phase#, hd_wippen#
Global hd_licht# = 1            ; Himmelslicht am Auge, so hell ist die Hand
Global hd_fackel.Fackel = Null  ; das Licht der Fackel in der Hand
Global hd_fackel_zeit           ; wann es zuletzt mitgewandert ist
Const HD_FACKEL_MS = 120        ; hoechstens so oft (jedes Mal bauen Chunks neu)
Dim hd_bloecke(D_ANZAHL - 1)    ; je Ding ein Mesh, erst bei Bedarf gebaut

Function Hand_Laden()
	hd_halter = CreatePivot(sp_kamera)
	; der Arm: lang nach vorn, zur Bildmitte und leicht nach oben geneigt
	hd_arm = Kloetzchen(hd_halter, 0, 0, 0, 0.2, 0.2, 0.75, 230, 185, 150)
	PositionEntity hd_arm, 0.04, -0.06, -0.04
	RotateEntity hd_arm, -12, 18, -8
	EntityOrder hd_arm, -1
	Licht_Merken(hd_arm, hd_arm)
	HideEntity hd_arm               ; Hand_Takt zeigt Arm oder Block
End Function

Function Hand_Zeigen(an)
	hd_zeigen = an
	If an Then ShowEntity hd_halter Else HideEntity hd_halter
End Function

; Einen Schlag beginnen. Jedes Bild bei gehaltener Taste aufrufen: der
; naechste Schlag beginnt, sobald der vorige zu Ende ist.
Function Hand_Schlagen()
	If hd_schlag < 0 Then hd_schlag = 0
End Function

; Ein Block in der Hand: kleiner Wuerfel mit den Texturen der Blockart,
; die Seiten unterschiedlich hell wie in der Welt.
Function Hand_Block(b)
	If hd_bloecke(b) Then Return hd_bloecke(b)
	If Ding_Flach(b) >= 0 Then Return Hand_Pflanze(b)
	m = CreateMesh(hd_halter)
	Restore hd_wuerfel
	For f = 0 To 5
		Read seite, hell#
		s = CreateSurface(m, tx_pinsel(bl_tex(b, seite)))
		For k = 0 To 3
			Read x#, y#, z#, u#, v#
			i = AddVertex(s, x * 0.5, y * 0.5, z * 0.5, u, v)
			VertexColor s, i, 255 * hell, 255 * hell, 255 * hell
			VertexTexCoords s, i, 0, 0.5, 0, 1  ; kein Fackellicht (Lichtrampe links)
		Next
		AddTriangle s, 0, 1, 2
		AddTriangle s, 0, 2, 3
	Next
	ScaleEntity m, 0.22, 0.22, 0.22
	PositionEntity m, 0.03, 0.0, 0.14
	RotateEntity m, 0, 45, 0
	EntityOrder m, -1
	HideEntity m
	hd_bloecke(b) = m
	Licht_Merken(m, m)
	Return m
End Function

; Eine Pflanze oder ein Gegenstand in der Hand: flach, leicht schraeg, beidseitig (der Pinsel
; der Pflanzentextur zeichnet beide Seiten).
Function Hand_Pflanze(b)
	m = CreateMesh(hd_halter)
	s = CreateSurface(m, tx_pinsel(Ding_Flach(b)))
	AddVertex s, -0.5, 0.5, 0, 0, 0
	AddVertex s, 0.5, 0.5, 0, 1, 0
	AddVertex s, 0.5, -0.5, 0, 1, 1
	AddVertex s, -0.5, -0.5, 0, 0, 1
	For i = 0 To 3
		VertexColor s, i, 255, 255, 255
		VertexTexCoords s, i, 0, 0.5, 0, 1  ; kein Fackellicht (Lichtrampe links)
	Next
	AddTriangle s, 0, 1, 2
	AddTriangle s, 0, 2, 3
	If Ist_Block(b)
		ScaleEntity m, 0.34, 0.34, 0.34
		PositionEntity m, 0.0, 0.06, 0.14
		RotateEntity m, 0, -25, 0
	Else
		; Gegenstaende: der Griff (in der Textur links unten) liegt rechts
		; unten in der Hand, Kopf bzw. Spitze zeigen nach oben zur Bildmitte
		ScaleEntity m, 0.3, 0.3, 0.3
		PositionEntity m, -0.02, 0.12, 0.16
		RotateEntity m, 0, -25, 90
	EndIf
	EntityOrder m, -1
	HideEntity m
	hd_bloecke(b) = m
	Licht_Merken(m, m)
	Return m
End Function

; Flaechen des Wuerfels: Textur (0 oben, 1 Seite, 2 unten), Helligkeit, dann
; vier Ecken x y z u v, von aussen gesehen im Uhrzeigersinn
.hd_wuerfel
Data 0, 1.0,   -1, 1, 1, 0, 0,   1, 1, 1, 1, 0,   1, 1, -1, 1, 1,   -1, 1, -1, 0, 1
Data 2, 0.5,   -1, -1, -1, 0, 0,   1, -1, -1, 1, 0,   1, -1, 1, 1, 1,   -1, -1, 1, 0, 1
Data 1, 0.8,   -1, 1, -1, 0, 0,   1, 1, -1, 1, 0,   1, -1, -1, 1, 1,   -1, -1, -1, 0, 1
Data 1, 0.8,   1, 1, 1, 0, 0,   -1, 1, 1, 1, 0,   -1, -1, 1, 1, 1,   1, -1, 1, 0, 1
Data 1, 0.65,  -1, 1, 1, 0, 0,   -1, 1, -1, 1, 0,   -1, -1, -1, 1, 1,   -1, -1, 1, 0, 1
Data 1, 0.65,  1, 1, -1, 0, 0,   1, 1, 1, 1, 0,   1, -1, 1, 1, 1,   1, -1, -1, 0, 1

; Das Licht der Fackel in der Hand: an der Zelle des Auges, wandert mit,
; sobald das Auge in eine andere Zelle kommt (hoechstens alle
; HD_FACKEL_MS). Alter und neuer Lichtbereich bauen neu.
Function Handlicht(an, x#, y#, z#)
	bx = Floor(x) : by = Floor(y) : bz = Floor(z)
	If an
		If hd_fackel = Null
			hd_fackel = New Fackel
			hd_fackel\hand = True
			hd_fackel\licht = CreateBank(FK_VOLUMEN)
			hd_fackel\x = bx : hd_fackel\y = -1000 : hd_fackel\z = bz
			hd_fackel_zeit = 0
		EndIf
		If (bx <> hd_fackel\x Or by <> hd_fackel\y Or bz <> hd_fackel\z) And MilliSecs() - hd_fackel_zeit >= HD_FACKEL_MS
			hd_fackel_zeit = MilliSecs()
			Fackel_Chunks_Veralten(hd_fackel\x, hd_fackel\z)
			hd_fackel\x = bx : hd_fackel\y = by : hd_fackel\z = bz
			Fackel_Leuchten(hd_fackel)
			Fackel_Chunks_Veralten(bx, bz)
		EndIf
	ElseIf hd_fackel <> Null
		Fackel_Chunks_Veralten(hd_fackel\x, hd_fackel\z)
		FreeBank hd_fackel\licht
		Delete hd_fackel
		hd_fackel = Null
	EndIf
End Function

Function Hand_Takt(dt#)
	; Fackel in der Hand: leuchtet nur, wo es dunkel genug ist, dass man es
	; sieht - am hellen Tag im Freien spart das den Neubau. Zwei Schwellen,
	; damit es an der Grenze nicht flackert. Auch mit versteckter Hand
	; (Rundflug), damit die Messung den Neubau enthaelt.
	kx# = EntityX(sp_kamera) : ky# = EntityY(sp_kamera) : kz# = EntityZ(sp_kamera)
	an = False
	If Inventar_Gewaehlt() = B_FACKEL
		hell# = Himmel_Bei(kx, ky, kz) * nz_umgebung
		If hell < 0.55 Or (hd_fackel <> Null And hell < 0.65) Then an = True
	EndIf
	Handlicht(an, kx, ky, kz)
	If Not hd_zeigen Then Return

	; was gerade gewaehlt ist
	b = Inventar_Gewaehlt()
	If b <> hd_block
		If hd_block > 0 Then HideEntity hd_bloecke(hd_block)
		If hd_block = B_LUFT Then HideEntity hd_arm
		If b = B_LUFT
			ShowEntity hd_arm
		Else
			ShowEntity Hand_Block(b)
		EndIf
		If hd_block >= 0 Then hd_wechsel = 1
		hd_block = b
		hd_licht = -1               ; die neue Hand gleich abdunkeln
	EndIf

	; in einer Hoehle ist auch die Hand dunkel
	l# = Licht_Bei(kx, ky, kz)
	If Abs(l - hd_licht) > 0.01
		hd_licht = l
		If hd_block = B_LUFT Then Licht_Setzen(hd_arm, l) Else Licht_Setzen(hd_bloecke(hd_block), l)
	EndIf
	hd_wechsel = hd_wechsel - dt * 4
	If hd_wechsel < 0 Then hd_wechsel = 0

	; Schlag: nach vorn zur Bildmitte stossen, an der Spitze leicht nach unten
	; kippen, dann zurueck. Kippt er staerker, verschwindet der Arm unten aus
	; dem Bild.
	If hd_schlag >= 0
		hd_schlag = hd_schlag + dt / SCHLAG_DAUER
		If hd_schlag >= 1 Then hd_schlag = -1
	EndIf
	sx# = 0 : sy# = 0 : sz# = 0 : sn# = 0 : sg# = 0
	If hd_schlag >= 0
		s1# = Sin(hd_schlag * 180)
		s2# = Sin(Sqr(hd_schlag) * 180)
		sx = -0.25 * s2
		sy = 0.08 * s2
		sz = 0.15 * s1
		sn = 20 * s1
		sg = 30 * s2
	EndIf

	; Wippen beim Laufen am Boden
	tempo# = Sqr(sp_vx * sp_vx + sp_vz * sp_vz)
	ziel# = 0
	If sp_boden And (Not sp_fliegen) And tempo > 0.5 Then ziel = 1
	hd_wippen = hd_wippen + (ziel - hd_wippen) * Klemmen(dt * 8, 0, 1)
	hd_phase = hd_phase + tempo * dt * 110
	wx# = Sin(hd_phase) * 0.025 * hd_wippen
	wy# = -Abs(Cos(hd_phase)) * 0.03 * hd_wippen

	PositionEntity hd_halter, HD_X + sx + wx, HD_Y + sy + wy - 0.35 * hd_wechsel * hd_wechsel, HD_Z + sz
	RotateEntity hd_halter, sn, sg, 0
End Function
