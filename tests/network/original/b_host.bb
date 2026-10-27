; Szenario B, Host: verlaesst das Spiel nach 15 s (Argument "stop") oder wird
; vom Skript beendet (Argument "kill") (Fragen 8, 9)
Include "netlog.bb"
LogInit()
r = HostNetGame("messb")
Note("HostNetGame=" + r)
If r <> 2 Then End
h = CreateNetPlayer("HB")
Note("CreateNetPlayer HB=" + h)
While MilliSecs() - T0 < 15000
	LogMsgs()
	Delay 5
Wend
If Arg$ = "stop"
	Note("StopNetGame vor")
	StopNetGame
	Note("StopNetGame nach")
Else
	Note("warte auf Abbruch durch Skript")
	Pump(60000)
EndIf
Note("ende")
End
