; Client nur mit IPv6: schreibt A (nur IPv4) und wartet auf die Antwort
Include "../common.bb"
NetGameApp "mixed"
NetGamePort 28768
Print "join " + JoinNetGame("mixed", Lo())
While RecvNetMsg() : Print "early " + Describe() : Wend
b1 = CreateNetPlayer("B1") : Remember(b1)
SendNetMsg 31, "B to A", b1, IdOf("A1")
StartClock(20000)
done = False
While (Not done) And (Not TimeUp())
	While RecvNetMsg()
		Print Describe()
		If NetMsgType() = 32 Then done = True
	Wend
	Delay 2
Wend
StopNetGame
Print "stopped"
