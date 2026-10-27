; Szenario A, Host: Ereignisse, Nachrichten an sich selbst, Abmelden (Fragen 1, 2, 3, 10)
Include "netlog.bb"
LogInit()
r = HostNetGame("mess")
Note("HostNetGame=" + r)
If r <> 2 Then End
h1 = CreateNetPlayer("H1")
Note("CreateNetPlayer H1=" + h1)
h2 = CreateNetPlayer("H2")
Note("CreateNetPlayer H2=" + h2)
Pump(300)
SendNetMsg 11, "H1 an H2", h1, h2, 1
Note("SendNetMsg 11 H1->H2 (lokal an lokal)")
SendNetMsg 12, "H1 an alle", h1, 0, 1
Note("SendNetMsg 12 H1->0 (an alle)")
Pump(500)
; bis 32 s: jeden neuen Spieler begruessen, direkt und an alle
While MilliSecs() - T0 < 32000
	prev = LastJoined
	LogMsgs()
	If LastJoined <> prev
		SendNetMsg 13, "H1 an alle nach Beitritt", h1, 0, 1
		SendNetMsg 14, "H1 direkt an neuen", h1, LastJoined, 1
		Note("SendNetMsg 13 an alle, 14 an " + LastJoined)
	EndIf
	Delay 5
Wend
Note("StopNetGame vor")
StopNetGame
Note("StopNetGame nach")
Pump(500)
Note("RecvNetMsg nach Stop=" + RecvNetMsg())
Note("ende")
End
