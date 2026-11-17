; Aeltester Client mit IPv4 und IPv6, tritt ueber IPv6 bei: wird neuer Host
Include "../common.bb"
NetGameApp "mmixed"
NetGamePort 28769
Print "join " + JoinNetGame("mmixed", Lo())
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
