; Host: wartet auf beide Clients und verlaesst dann das Spiel
Include "../common.bb"
NetGameApp "migrate"
NetGamePort 28763
Print "host " + HostNetGame("migrate")
h1 = CreateNetPlayer("H1") : Remember(h1)
StartClock(20000)
seen = 0
While seen < 2 And (Not TimeUp())
	While RecvNetMsg()
		If NetMsgType() = 100 Then seen = seen + 1
	Wend
	Delay 2
Wend
Delay 300
StopNetGame
Print "stopped"
