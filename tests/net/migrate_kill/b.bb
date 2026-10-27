; Zweiter Client: verbindet sich mit dem neuen Host und schickt eine
; zuverlaessige Nachricht, noch waehrend die Migration laeuft
Include "../common.bb"
NetGameApp "mkill"
NetGamePort 28764
Print "join " + JoinNetGame("mkill", "127.0.0.1")
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
