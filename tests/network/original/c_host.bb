; Szenario C, Host fuer die Beitrittsvarianten (Frage 4)
Include "netlog.bb"
LogInit()
r = HostNetGame("mess")
Note("HostNetGame=" + r)
If r <> 2 Then End
h = CreateNetPlayer("HC")
Note("CreateNetPlayer HC=" + h)
Pump(Int(Arg$))
StopNetGame
Note("ende")
End
