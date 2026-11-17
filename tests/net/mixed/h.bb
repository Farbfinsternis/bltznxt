; Host mit IPv4 und IPv6; A kommt nur ueber IPv4, B nur ueber IPv6
Include "../common.bb"
NetGameApp "mixed"
NetGamePort 28768
Print "host " + HostNetGame("mixed")
h1 = CreateNetPlayer("H1") : Remember(h1)
StartClock(20000)
done = False
While (Not done) And (Not TimeUp())
	While RecvNetMsg()
		Print Describe()
		If NetMsgType() = 101 And NameOf(NetMsgFrom()) = "A1" Then done = True
	Wend
	Delay 2
Wend
StopNetGame
Print "stopped"
