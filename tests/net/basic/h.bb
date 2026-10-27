; Host: zwei lokale Spieler, lokal an lokal, Begruessung neuer Spieler
Include "../common.bb"
NetGameApp "basic"
NetGamePort 28761
Print "host " + HostNetGame("basic")
h1 = CreateNetPlayer("H1") : Remember(h1)
h2 = CreateNetPlayer("H2") : Remember(h2)
Print "local " + NetPlayerLocal(h1) + " " + NetPlayerLocal(h2) + " " + NetGameHost()
SendNetMsg 40, "H1 to H2", h1, h2
SendNetMsg 41, "H1 to all", h1
StartClock(20000)
leftA = False
While (Not leftA) And (Not TimeUp())
	While RecvNetMsg()
		If NetMsgType() = 13
			Print "13 len " + Len(NetMsgData()) + " mid " + Asc(Mid(NetMsgData(), 2, 1))
		Else
			Print Describe()
		EndIf
		If NetMsgType() = 100
			If NameOf(NetMsgFrom()) = "A1"
				SendNetMsg 14, "host to all", h1
				SendNetMsg 15, "host to all unreliable", h1, 0, 0
			ElseIf NameOf(NetMsgFrom()) = "B1"
				SendNetMsg 30, "host to B1", h2, NetMsgFrom()
			EndIf
		EndIf
		If NetMsgType() = 101 And NameOf(NetMsgFrom()) = "A1" Then leftA = True
	Wend
	Delay 2
Wend
StopNetGame
Print "stopped"
