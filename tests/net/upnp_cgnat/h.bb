; Router kennt nur dauerhafte Weiterleitungen und steht selbst hinter
; Carrier-NAT: weitergeleitet, aber von aussen nicht erreichbar
Include "../common.bb"
NetGamePort 28767
Print "before " + NetGameForwarded()
Print "host " + HostNetGame("cgnat")
StartClock(8000)
While NetGameForwarded() = -1 And (Not TimeUp())
	Delay 10
Wend
Print "forwarded " + NetGameForwarded()
StopNetGame
Print "after stop " + NetGameForwarded()
s = OpenTCPStream("127.0.0.1", 28792)
While Not Eof(s)
	l$ = ReadLine(s)
	If l <> "" Then Print "router: " + l
Wend
CloseTCPStream s
