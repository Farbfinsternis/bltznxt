; Host bittet den (nachgebauten) Router um eine Weiterleitung und gibt sie
; beim Ende zurueck; danach das Protokoll des Routers
Include "../common.bb"
NetGamePort 28767
Print "before " + NetGameForwarded()
Print "host " + HostNetGame("upnp")
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
