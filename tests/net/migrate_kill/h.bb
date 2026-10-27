; Host, der nie geht: der Testlauf beendet den Prozess hart
NetGameApp "mkill"
NetGamePort 28764
HostNetGame("mkill")
h1 = CreateNetPlayer("H1")
While True
	While RecvNetMsg() : Wend
	Delay 5
Wend
