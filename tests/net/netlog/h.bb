; Host mit IPv4 und IPv6 schreibt ein Diagnose-Log (NetGameLog)
Include "../common.bb"
Include "logcheck.bb"
NetGameLog "bin/net_netlog/h.log"
NetGameServer "http://127.0.0.1:28790/games"
NetGamePort 28772
Print "host " + HostNetGame("lognet")
h = CreateNetPlayer("Hosti")
StartClock(25000)
done = False
While (Not done) And (Not TimeUp())
	While RecvNetMsg()
		If NetMsgType() = 101 Then done = True
	Wend
	Delay 2
Wend
StopNetGame
Delay 100
LogCheck("bin/net_netlog/h.log", "BLTZNXT network log, protocol 3|client sockets: IPv4 yes, IPv6 yes|hosting: game port 28772, IPv4 yes, IPv6 yes|list server: registered, IPv6 address reported|a program joined over IPv6 (IPv6 loopback)|a program left the game (left)|game stopped")
