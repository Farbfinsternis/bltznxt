; Host mit IPv4 und IPv6, der nie geht: der Testlauf beendet den Prozess hart
NetGameApp "mmixed"
NetGamePort 28769
HostNetGame("mmixed")
h1 = CreateNetPlayer("H1")
While True
	While RecvNetMsg() : Wend
	Delay 5
Wend
