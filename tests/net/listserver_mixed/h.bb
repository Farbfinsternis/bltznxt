; Host mit IPv4 und IPv6 meldet sich ueber IPv4 beim Listen-Server an und
; nennt seine IPv6-Adresse selbst (ip6=; der Testserver nimmt auch ::1 an)
Include "../common.bb"
NetGameServer "http://127.0.0.1:28790/games"
NetGamePort 28770
Print "host " + HostNetGame("lmixed")
h = CreateNetPlayer("H")
StartClock(30000)
done = False
While (Not done) And (Not TimeUp())
	While RecvNetMsg()
		Print Describe()
		If NetMsgType() = 101 And NameOf(NetMsgFrom()) = "CB" Then done = True
	Wend
	Delay 2
Wend
StopNetGame
Print "stopped"
