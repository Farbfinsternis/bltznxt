; Client nur mit IPv4: grueßt alle, antwortet B (nur IPv6) ueber den Host
Include "../common.bb"
NetGameApp "mixed"
NetGamePort 28768
Print "join " + JoinNetGame("mixed", Lo())
While RecvNetMsg() : Print "early " + Describe() : Wend
a1 = CreateNetPlayer("A1") : Remember(a1)
StartClock(20000)
done = False
While (Not done) And (Not TimeUp())
	While RecvNetMsg()
		Print Describe()
		If NetMsgType() = 100 And NameOf(NetMsgFrom()) = "B1" Then SendNetMsg 30, "A to all", a1
		If NetMsgType() = 31 Then SendNetMsg 32, "A to B", a1, NetMsgFrom()
		If NetMsgType() = 101 And NameOf(NetMsgFrom()) = "B1" Then done = True
	Wend
	Delay 2
Wend
StopNetGame
Print "stopped"
