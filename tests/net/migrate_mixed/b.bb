; Client nur mit IPv4: kennt den neuen Host nur ueber dessen IPv4-Adresse aus
; der Rechnerliste, nicht ueber die IPv6-Adresse, unter der der alte Host ihn sah
Include "../common.bb"
NetGameApp "mmixed"
NetGamePort 28769
Print "join " + JoinNetGame("mmixed", Lo())
While RecvNetMsg() : Print "early " + Describe() : Wend
b1 = CreateNetPlayer("B1") : Remember(b1)
StartClock(30000)
done = False
While (Not done) And (Not TimeUp())
	While RecvNetMsg()
		Print Describe()
		If NetMsgType() = 101 And NameOf(NetMsgFrom()) = "H1"
			SendNetMsg 21, "B after", b1, IdOf("A1")
			Print "host " + NetGameHost()
		EndIf
		If NetMsgType() = 22 Then done = True
	Wend
	Delay 2
Wend
StopNetGame
Print "stopped"
