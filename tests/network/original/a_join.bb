; Szenario A, Beitretende J1 und J2 (Argument = Spielername)
Include "netlog.bb"
LogInit()
r = JoinNetGame("mess", "127.0.0.1")
Note("JoinNetGame=" + r)
If r <> 1 Then End
Note("vor CreateNetPlayer: Nachrichten?")
Pump(500)
p = CreateNetPlayer(Arg$)
Note("CreateNetPlayer " + Arg$ + "=" + p)
Pump(1000)
SendNetMsg 21, Arg$ + " an ersten", p, FirstRemote, 1
SendNetMsg 22, Arg$ + " an alle", p, 0, 1
SendNetMsg 23, Arg$ + " an alle unzuv", p, 0, 0
Note("SendNetMsg 21 an " + FirstRemote + ", 22 an alle, 23 an alle unzuverlaessig")
If Arg$ = "J1"
	Pump(4000)
	Note("DeleteNetPlayer vor")
	DeleteNetPlayer p
	Note("DeleteNetPlayer nach")
	Pump(1000)
	Note("NetPlayerName geloeschter eigener=[" + NetPlayerName$(p) + "] local=" + NetPlayerLocal(p))
	Note("StopNetGame vor")
	StopNetGame
	Note("StopNetGame nach")
	r = JoinNetGame("mess", "127.0.0.1")
	Note("JoinNetGame erneut=" + r)
	If r = 1
		p = CreateNetPlayer("J1b")
		Note("CreateNetPlayer J1b=" + p)
		Pump(2000)
		StopNetGame
		Note("StopNetGame 2")
	EndIf
Else
	Pump(20000)
	Note("StopNetGame vor")
	StopNetGame
	Note("StopNetGame nach")
EndIf
Note("ende")
End
