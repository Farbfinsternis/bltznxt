; BLTZNXT NetDemo - Mehrspieler mit den Net-Befehlen (NETWORKING.md).
;
; Zuerst eine kleine Lobby: FindNetGames listet die Spiele im LAN (und beim
; Listen-Server, falls einer eingestellt ist). 1-9 tritt bei, H hostet, R sucht
; neu. Ohne Taste entscheidet nach 10 Sekunden StartNetGame: beitreten, wenn
; es ein Spiel gibt, sonst hosten.
;
; Im Spiel steuert jeder sein Quadrat mit den Pfeiltasten. Die Position geht
; 20-mal je Sekunde unzuverlaessig an alle (Typ 1), eine Nachricht mit der
; Leertaste zuverlaessig (Typ 2). 100/101/102 zeigen, wer kommt, wer geht und
; wann dieses Programm Host wird. Esc beendet.
;
; Zum Ausprobieren das Programm zweimal starten, auf einem Rechner oder auf
; zweien im selben Netz.

Graphics 640, 480, 0, 2
SetBuffer BackBuffer()
AppTitle "BLTZNXT NetDemo"
NetGameApp "netdemo"
SeedRnd MilliSecs()

Type Mover
	Field id, name$
	Field x#, y#
End Type

Global Info$ = "", InfoTime = 0

Function Say(t$)
	Info = t
	InfoTime = MilliSecs()
End Function

Function MoverOf.Mover(id)
	For m.Mover = Each Mover
		If m\id = id Then Return m
	Next
	Return Null
End Function

; ---- Lobby ----------------------------------------------------------------

Text 10, 10, "Suche Spiele ..."
Flip
n = FindNetGames(1000)
choice = -1     ; -1 noch offen, 0 hosten, 1..9 beitreten, 99 StartNetGame
t0 = MilliSecs()
While choice < 0
	Cls
	Color 255, 255, 255
	Text 10, 10, "Spiele im Netz:"
	If n = 0 Then Text 20, 34, "(keine gefunden)"
	For i = 1 To n
		Text 20, 18 + i * 16, i + ": " + NetGameName(i) + "  (" + NetGamePlayers(i) + " Spieler, " + NetGameIP(i) + ")"
	Next
	rest = 10 - (MilliSecs() - t0) / 1000
	Color 180, 180, 180
	Text 10, 400, "1-9 beitreten, H hosten, R neu suchen, Esc Ende"
	Text 10, 420, "Ohne Taste in " + rest + " s automatisch (StartNetGame)"
	Flip
	k = GetKey()
	If k >= 49 And k <= 57 And k - 48 <= n Then choice = k - 48
	If k = 72 Or k = 104 Then choice = 0
	If k = 82 Or k = 114
		Cls : Text 10, 10, "Suche Spiele ..." : Flip
		n = FindNetGames(1000)
		t0 = MilliSecs()
	EndIf
	If KeyHit(1) Then End
	If rest <= 0 Then choice = 99
	Delay 10
Wend

Cls : Text 10, 10, "Verbinde ..." : Flip
Select choice
Case 99
	r = StartNetGame()
Case 0
	r = HostNetGame("NetDemo")
Default
	r = JoinNetGame(NetGameName(choice), NetGameIP(choice))
End Select
If r = 0 Then RuntimeError "Kein Netzwerkspiel - Gruende stehen auf der Konsole"

name$ = "Spieler " + Rand(100, 999)
me.Mover = New Mover
me\id = CreateNetPlayer(name)
me\name = name
me\x = Rand(40, 600)
me\y = Rand(60, 440)
If r = 2 Then Say("Spiel gehostet") Else Say("Spiel beigetreten")

; ---- Spiel ----------------------------------------------------------------

lastSend = 0
While Not KeyHit(1)
	If KeyDown(203) Then me\x = me\x - 3
	If KeyDown(205) Then me\x = me\x + 3
	If KeyDown(200) Then me\y = me\y - 3
	If KeyDown(208) Then me\y = me\y + 3
	If me\x < 0 Then me\x = 0
	If me\x > 620 Then me\x = 620
	If me\y < 40 Then me\y = 40
	If me\y > 460 Then me\y = 460

	If MilliSecs() - lastSend >= 50
		SendNetMsg 1, Int(me\x) + "," + Int(me\y), me\id, 0, 0
		lastSend = MilliSecs()
	EndIf
	If KeyHit(57) Then SendNetMsg 2, "Hallo von " + me\name, me\id

	While RecvNetMsg()
		Select NetMsgType()
		Case 100
			m.Mover = New Mover
			m\id = NetMsgFrom()
			m\name = NetPlayerName(NetMsgFrom())
			m\x = -100
			Say(m\name + " ist da")
		Case 101
			m = MoverOf(NetMsgFrom())
			If m <> Null Then Say(m\name + " ist gegangen") : Delete m
		Case 102
			Say("Dieses Programm ist jetzt Host")
		Case 200
			Say("Verbindung verloren")
		Case 1
			m = MoverOf(NetMsgFrom())
			If m <> Null
				d$ = NetMsgData()
				c = Instr(d, ",")
				m\x = Int(Left(d, c - 1))
				m\y = Int(Mid(d, c + 1))
			EndIf
		Case 2
			Say(NetMsgData())
		End Select
	Wend

	Cls
	For m.Mover = Each Mover
		If m = me Then Color 255, 220, 0 Else Color 0, 180, 255
		Rect m\x, m\y, 20, 20
		Color 255, 255, 255
		Text m\x - 10, m\y - 16, m\name
	Next
	Color 255, 255, 255
	Text 10, 10, me\name + "   Host: " + NetGameHost() + "   Pfeiltasten, Leertaste, Esc"
	If MilliSecs() - InfoTime < 3000 Then Text 10, 26, Info
	Flip
Wend

StopNetGame
End
