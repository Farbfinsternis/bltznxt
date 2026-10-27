; Aeltester Client: wird neuer Host
Include "../common.bb"
NetGameApp "migrate"
NetGamePort 28763
Print "join " + JoinNetGame("migrate", "127.0.0.1")
While RecvNetMsg() : Print "early " + Describe() : Wend
a1 = CreateNetPlayer("A1") : Remember(a1)
StartClock(30000)
done = False
While (Not done) And (Not TimeUp())
	While RecvNetMsg()
		Print Describe()
		If NetMsgType() = 102 Then Print "host " + NetGameHost()
		If NetMsgType() = 21 Then SendNetMsg 22, "A ack", a1, NetMsgFrom()
		If NetMsgType() = 101 And NameOf(NetMsgFrom()) = "B1" Then done = True
	Wend
	Delay 2
Wend
StopNetGame
Print "stopped"
