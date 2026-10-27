; Erster Client: Nachrichten an einen Spieler, an alle, mit Chr(0)
Include "../common.bb"
NetGameApp "basic"
NetGamePort 28761
Print "join " + JoinNetGame("basic", "127.0.0.1")
While RecvNetMsg()
	Print "early " + Describe()
Wend
a1 = CreateNetPlayer("A1") : Remember(a1)
SendNetMsg 11, "A1 to H1", a1, IdOf("H1")
SendNetMsg 12, "A1 to all", a1
SendNetMsg 13, "x" + Chr(0) + "y", a1, IdOf("H1")
StartClock(20000)
done = False
While (Not done) And (Not TimeUp())
	While RecvNetMsg()
		Print Describe()
		If NetMsgType() = 101 And NameOf(NetMsgFrom()) = "B1" Then done = True
	Wend
	Delay 2
Wend
StopNetGame
Print "stopped"
