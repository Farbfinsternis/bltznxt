; Szenario B, Beitretende: protokollieren den Wegfall des Hosts (Argument = Name)
Include "netlog.bb"
LogInit()
r = JoinNetGame("messb", "127.0.0.1")
Note("JoinNetGame=" + r)
If r <> 1 Then End
p = CreateNetPlayer(Arg$)
Note("CreateNetPlayer " + Arg$ + "=" + p)
n = 0
lastTick = MilliSecs()
seen102 = 0
While MilliSecs() - T0 < 34000
	LogMsgs()
	If MilliSecs() - lastTick >= 2000
		n = n + 1
		SendNetMsg 31, Arg$ + " tick " + n, p, 0, 1
		lastTick = MilliSecs()
	EndIf
	If Got102 > seen102
		seen102 = Got102
		SendNetMsg 32, Arg$ + " ist neuer Host", p, 0, 1
		Note("SendNetMsg 32 (neuer Host)")
	EndIf
	Delay 5
Wend
Note("StopNetGame vor")
StopNetGame
Note("StopNetGame nach")
Note("ende")
End
