; BLTZCRFT - Mehrspieler
;
; Mehrere Spieler laufen durch dieselbe Welt, sehen einander als
; Kloetzchenfigur mit Namen und schreiben sich im Chat (T). Abgebaute und
; gesetzte Bloecke sehen alle; sonst wirkt niemand auf den anderen.
;
; Darunter liegen die Net-Befehle von BLTZNXT (NETWORKING.md im Hauptordner).
; Ein Programm hostet, die anderen treten bei. Eine Nachricht an alle
; (Empfaenger 0) reicht der Host an die anderen weiter. Nachrichten:
;
;   1 Hallo    Host an den Neuen, zuverlaessig: "saat x y z" - die Saat der
;              Welt und wo der Host steht (in Hundertstel Bloecken). Gleich
;              danach die bisherigen Aenderungen als 3.
;   2 Stand    an alle, unzuverlaessig, 15-mal je Sekunde:
;              "x y z gier neig", Position in Hundertstel Bloecken
;   3 Bloecke  zuverlaessig: "x y z b" je Block, mehrere durch ";" getrennt
;   4 Chat     an alle, zuverlaessig: der Text
;
; Die Welt entsteht ueberall gleich aus der Saat; zu uebertragen sind nur die
; Aenderungen (welt.bb). Jedes Programm hat die ganze Liste - so kann nach
; einem Host-Wechsel (102) der neue Host Neuankoemmlinge versorgen.
; Bauen zwei Spieler im selben Augenblick an derselben Stelle, kann die
; Reihenfolge bei beiden verschieden sein; fuer ein Beispiel reicht das.

Const MP_SPIEL$ = "bltzcrft"
Const MP_STAND_MS = 66      ; so oft geht die eigene Position hinaus
Const MP_BLOCK_JE = 300     ; Bloecke je Nachricht beim Nachreichen
Const MP_NAMEN_WEIT = 48    ; bis zu dieser Entfernung steht der Name ueber der Figur

Global mp_an = False        ; im Mehrspieler
Global mp_ich               ; eigene Spieler-ID
Global mp_name$
Global mp_saat, mp_hx#, mp_hy#, mp_hz#, mp_hallo
Global mp_gesendet

Global chat_offen = False
Global chat_text$

Type Mitspieler
	Field id, name$
	Field figur, kopf, arm_l, arm_r, bein_l, bein_r
	Field x#, y#, z#, gier#, neig#          ; so steht die Figur
	Field zx#, zy#, zz#, zgier#, zneig#     ; zuletzt empfangen
	Field schritt#
	Field neu                               ; noch keine Position bekommen
End Type

Type ChatZeile
	Field text$, bis, r, g, b
End Type

; Ein Spiel im LAN suchen und beitreten, sonst selbst hosten. Mit ip$ nur
; dort beitreten. 1 beigetreten (mp_saat, mp_hx/hy/hz vom Host), 2 Host,
; 0 nichts davon.
Function Mp_Starten(name$, ip$, saat)
	mp_name = name
	NetGameApp MP_SPIEL
	r = 0
	If ip <> ""
		r = JoinNetGame("", ip)
	Else
		n = FindNetGames(1000)
		For i = 1 To n
			If JoinNetGame(NetGameName(i), NetGameIP(i)) Then r = 1 : Exit
		Next
	EndIf
	If r = 0 And ip = ""
		If HostNetGame("BLTZCRFT " + name) Then r = 2
	EndIf
	If r = 0 Then Return 0
	mp_an = True
	mp_ich = CreateNetPlayer(name)
	mp_saat = saat
	If r = 2 Then Return 2
	; auf das Hallo des Hosts warten
	mp_hallo = False
	ende = MilliSecs() + 5000
	While (Not mp_hallo) And MilliSecs() < ende And mp_an
		Mp_Empfangen()
		Delay 10
	Wend
	If mp_hallo Then Return 1
	Mp_Beenden()
	Return 0
End Function

Function Mp_Beenden()
	If mp_an Then StopNetGame
	mp_an = False
	For m.Mitspieler = Each Mitspieler
		FreeEntity m\figur
		Delete m
	Next
End Function

; Jedes Bild: Nachrichten holen, die eigene Position senden, Figuren nachziehen.
Function Mp_Takt(dt#)
	If Not mp_an Then Return
	Mp_Empfangen()
	If Not mp_an Then Return
	If MilliSecs() - mp_gesendet >= MP_STAND_MS
		mp_gesendet = MilliSecs()
		SendNetMsg 2, Int(sp_x * 100) + " " + Int(sp_y * 100) + " " + Int(sp_z * 100) + " " + Int(sp_gier * 100) + " " + Int(sp_neig * 100), mp_ich, 0, False
	EndIf
	For m.Mitspieler = Each Mitspieler
		Figur_Takt(m, dt)
	Next
End Function

; Ein eigener Block geht an alle.
Function Mp_Block(x, y, z, b)
	If mp_an Then SendNetMsg 3, x + " " + y + " " + z + " " + b, mp_ich
End Function

Function Mp_Empfangen()
	While RecvNetMsg()
		von = NetMsgFrom()
		Select NetMsgType()
		Case 1
			d$ = NetMsgData()
			mp_saat = Int(Wort(d, 1))
			mp_hx = Int(Wort(d, 2)) / 100.0
			mp_hy = Int(Wort(d, 3)) / 100.0
			mp_hz = Int(Wort(d, 4)) / 100.0
			mp_hallo = True
		Case 2
			m.Mitspieler = Mp_Finden(von)
			If m <> Null
				d$ = NetMsgData()
				m\zx = Int(Wort(d, 1)) / 100.0
				m\zy = Int(Wort(d, 2)) / 100.0
				m\zz = Int(Wort(d, 3)) / 100.0
				m\zgier = Int(Wort(d, 4)) / 100.0
				m\zneig = Int(Wort(d, 5)) / 100.0
				If m\neu
					m\x = m\zx : m\y = m\zy : m\z = m\zz : m\gier = m\zgier : m\neig = m\zneig
					m\neu = False
					ShowEntity m\figur
				EndIf
			EndIf
		Case 3
			Bloecke_Lesen(NetMsgData())
		Case 4
			Chat_Zeile(Mp_Wer(von) + ": " + NetMsgData(), 255, 255, 255)
		Case 100
			If Not NetPlayerLocal(von)
				Mp_Neu(von)
				Chat_Zeile(NetPlayerName(von) + " ist dabei", 255, 220, 120)
				If NetGameHost() Then Mp_Willkommen(von)
			EndIf
		Case 101
			m.Mitspieler = Mp_Finden(von)
			If m <> Null
				Chat_Zeile(m\name + " ist gegangen", 255, 220, 120)
				FreeEntity m\figur
				Delete m
			EndIf
		Case 102
			Chat_Zeile("Der Host ist weg - du bist jetzt Host", 255, 220, 120)
		Case 200
			Chat_Zeile("Verbindung verloren - weiter allein", 255, 140, 120)
			Mp_Beenden()
			Return
		End Select
	Wend
End Function

; Der Host versorgt einen Neuen: Saat, eigene Position, alle Aenderungen.
Function Mp_Willkommen(id)
	SendNetMsg 1, mp_saat + " " + Int(sp_x * 100) + " " + Int(sp_y * 100) + " " + Int(sp_z * 100), mp_ich, id
	d$ = "" : n = 0
	For a.Aenderung = Each Aenderung
		If n > 0 Then d = d + ";"
		d = d + a\x + " " + a\y + " " + a\z + " " + a\b
		n = n + 1
		If n = MP_BLOCK_JE
			SendNetMsg 3, d, mp_ich, id
			d = "" : n = 0
		EndIf
	Next
	If n > 0 Then SendNetMsg 3, d, mp_ich, id
End Function

Function Bloecke_Lesen(d$)
	While d <> ""
		p = Instr(d, ";")
		If p
			e$ = Left(d, p - 1) : d = Mid(d, p + 1)
		Else
			e$ = d : d = ""
		EndIf
		Welt_Aendern(Int(Wort(e, 1)), Int(Wort(e, 2)), Int(Wort(e, 3)), Int(Wort(e, 4)))
	Wend
End Function

Function Mp_Finden.Mitspieler(id)
	For m.Mitspieler = Each Mitspieler
		If m\id = id Then Return m
	Next
	Return Null
End Function

Function Mp_Wer$(id)
	If id = mp_ich Then Return mp_name
	m.Mitspieler = Mp_Finden(id)
	If m <> Null Then Return m\name
	Return NetPlayerName(id)
End Function

; n-tes Wort (ab 1), Woerter durch ein Leerzeichen getrennt
Function Wort$(s$, n)
	For i = 2 To n
		p = Instr(s, " ")
		If p = 0 Then Return ""
		s = Mid(s, p + 1)
	Next
	p = Instr(s, " ")
	If p Then Return Left(s, p - 1)
	Return s
End Function

; ---- Figur -----------------------------------------------------------------

; Kloetzchenfigur, 1,8 Bloecke hoch wie der Spieler; der Drehpunkt liegt
; zwischen den Fuessen. Hemdfarbe aus der Spieler-ID.
Function Mp_Neu(id)
	If Mp_Finden(id) <> Null Then Return
	m.Mitspieler = New Mitspieler
	m\id = id
	m\name = NetPlayerName(id)
	m\neu = True
	h = ((id Xor (id Shr 16)) * 40503) Shr 4
	hr = 60 + (h And 127) : hg = 60 + ((h Shr 7) And 127) : hb = 60 + ((h Shr 14) And 127)
	m\figur = CreatePivot()
	; Beine und Arme haengen an einem Gelenk, damit sie schwingen koennen
	m\bein_l = Glied(m\figur, -0.125, 0.7, 0, 0.24, 0.7, 0.24, 50, 60, 120)
	m\bein_r = Glied(m\figur, 0.125, 0.7, 0, 0.24, 0.7, 0.24, 50, 60, 120)
	Kloetzchen(m\figur, 0, 1.05, 0, 0.5, 0.7, 0.26, hr, hg, hb)
	m\arm_l = Glied(m\figur, -0.35, 1.38, 0, 0.2, 0.66, 0.2, hr, hg, hb)
	m\arm_r = Glied(m\figur, 0.35, 1.38, 0, 0.2, 0.66, 0.2, hr, hg, hb)
	m\kopf = CreatePivot(m\figur)
	PositionEntity m\kopf, 0, 1.4, 0
	Kloetzchen(m\kopf, 0, 0.2, 0, 0.4, 0.4, 0.4, 230, 185, 150)
	Kloetzchen(m\kopf, 0, 0.36, 0, 0.42, 0.1, 0.42, 90, 60, 40)          ; Haar
	Kloetzchen(m\kopf, -0.09, 0.22, 0.2, 0.07, 0.07, 0.02, 30, 30, 50)   ; Augen, vorn ist +z
	Kloetzchen(m\kopf, 0.09, 0.22, 0.2, 0.07, 0.07, 0.02, 30, 30, 50)
	HideEntity m\figur
End Function

; Gelenk bei (x, y, z), darunter ein Quader der Laenge h
Function Glied(eltern, x#, y#, z#, b#, h#, t#, r, g, b2)
	gelenk = CreatePivot(eltern)
	PositionEntity gelenk, x, y, z
	Kloetzchen(gelenk, 0, -h / 2, 0, b, h, t, r, g, b2)
	Return gelenk
End Function

; Quader mit Mitte (x, y, z) und Kantenlaengen b, h, t. Licht gibt es in der
; Welt keines; die Helligkeit je Seite steckt wie bei den Bloecken in den
; Vertexfarben (oben hell, unten dunkel).
Function Kloetzchen(eltern, x#, y#, z#, b#, h#, t#, r, g, b2)
	w = CreateCube(eltern)
	ScaleMesh w, b / 2, h / 2, t / 2
	PositionEntity w, x, y, z
	s = GetSurface(w, 1)
	For v = 0 To CountVertices(s) - 1
		ny# = VertexNY(s, v) : nx# = VertexNX(s, v)
		hell# = 0.8
		If ny > 0.5 Then hell = 1.0
		If ny < -0.5 Then hell = 0.5
		If Abs(nx) > 0.5 Then hell = 0.65
		VertexColor s, v, r * hell, g * hell, b2 * hell
	Next
	EntityFX w, 1 + 2
	Return w
End Function

; Weich zur letzten empfangenen Stellung, Beine und Arme schwingen beim Gehen.
Function Figur_Takt(m.Mitspieler, dt#)
	If m\neu Then Return
	k# = dt * 12 : If k > 1 Then k = 1
	ax# = m\x : az# = m\z
	m\x = m\x + (m\zx - m\x) * k
	m\y = m\y + (m\zy - m\y) * k
	m\z = m\z + (m\zz - m\z) * k
	dg# = m\zgier - m\gier
	While dg > 180 : dg = dg - 360 : Wend
	While dg < -180 : dg = dg + 360 : Wend
	m\gier = m\gier + dg * k
	m\neig = m\neig + (m\zneig - m\neig) * k
	weg# = Sqr((m\x - ax) * (m\x - ax) + (m\z - az) * (m\z - az))
	m\schritt = m\schritt + weg * 120
	If weg < 0.002 Then m\schritt = m\schritt * 0.85     ; im Stehen zur Ruhe
	w# = Sin(m\schritt) * 40
	If dt > 0 And weg / dt < 1.0 Then w = w * weg / dt
	PositionEntity m\figur, m\x, m\y, m\z
	RotateEntity m\figur, 0, m\gier, 0
	RotateEntity m\kopf, m\neig, 0, 0
	RotateEntity m\bein_l, w, 0, 0
	RotateEntity m\bein_r, -w, 0, 0
	RotateEntity m\arm_l, -w, 0, 0
	RotateEntity m\arm_r, w, 0, 0
End Function

; Nach RenderWorld: die Namen ueber den Koepfen.
Function Mp_Namen_Zeichnen(kamera)
	For m.Mitspieler = Each Mitspieler
		If Not m\neu
			dx# = m\x - sp_x : dy# = m\y - sp_y : dz# = m\z - sp_z
			If dx * dx + dy * dy + dz * dz < MP_NAMEN_WEIT * MP_NAMEN_WEIT
				CameraProject kamera, m\x, m\y + 2.05, m\z
				If ProjectedZ() > 0 Then Text_Schatten(ProjectedX(), ProjectedY(), m\name, 255, 255, 255)
			EndIf
		EndIf
	Next
End Function

; ---- Chat ------------------------------------------------------------------

Function Chat_Zeile(t$, r, g, b)
	z.ChatZeile = New ChatZeile
	z\text = t : z\bis = MilliSecs() + 10000
	z\r = r : z\g = g : z\b = b
	; nur die letzten 20 behalten
	n = 0
	For z = Each ChatZeile : n = n + 1 : Next
	While n > 20
		Delete First ChatZeile
		n = n - 1
	Wend
End Function

Function Chat_Oeffnen()
	chat_offen = True
	chat_text = ""
	FlushKeys
End Function

Function Chat_Schliessen()
	chat_offen = False
	FlushKeys
End Function

; Tasten in die Eingabezeile; Enter schickt, Esc verwirft.
Function Chat_Tippen()
	k = GetKey()
	While k
		If k = 13
			If Trim(chat_text) <> ""
				SendNetMsg 4, chat_text, mp_ich
				Chat_Zeile(mp_name + ": " + chat_text, 255, 255, 255)
			EndIf
			Chat_Schliessen()
			Return
		ElseIf k = 27
			Chat_Schliessen()
			Return
		ElseIf k = 8
			If Len(chat_text) > 0 Then chat_text = Left(chat_text, Len(chat_text) - 1)
		ElseIf k >= 32 And Len(chat_text) < 80
			chat_text = chat_text + Chr(k)
		EndIf
		k = GetKey()
	Wend
End Function

; Unten links ueber der Schnellleiste: die neuesten Zeilen, beim Tippen alle.
Function Chat_Zeichnen()
	y = GraphicsHeight() - INV_FELD - 110
	If chat_offen
		Color 0, 0, 0 : Rect 10, y, 520, 22, 1
		Color 255, 255, 255 : Text 16, y + 4, "> " + chat_text + "_"
	EndIf
	z.ChatZeile = Last ChatZeile
	n = 0
	While z <> Null And n < 8
		If chat_offen Or MilliSecs() < z\bis
			y = y - 20
			Color 0, 0, 0 : Text 17, y + 1, z\text
			Color z\r, z\g, z\b : Text 16, y, z\text
			n = n + 1
		EndIf
		z = Before z
	Wend
End Function
