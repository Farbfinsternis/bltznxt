; Szenario C, eine Beitrittsvariante je Lauf (Argument "spielname|ip")
Include "netlog.bb"
LogInit()
name$ = Part$(Arg$, 0)
ip$ = Part$(Arg$, 1)
Note("JoinNetGame vor name=[" + name$ + "] ip=[" + ip$ + "]")
r = JoinNetGame(name$, ip$)
Note("JoinNetGame=" + r)
If r = 1
	p = CreateNetPlayer("JC")
	Note("CreateNetPlayer=" + p)
	Pump(500)
	StopNetGame
	Note("StopNetGame")
EndIf
Note("ende")
End
