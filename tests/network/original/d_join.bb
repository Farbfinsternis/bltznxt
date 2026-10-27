; Szenario D, Empfaenger: zaehlt, was ankommt
Include "netlog.bb"
LogInit()
r = JoinNetGame("messd", "127.0.0.1")
Note("JoinNetGame=" + r)
If r <> 1 Then End
p = CreateNetPlayer("JD")
Note("CreateNetPlayer JD=" + p)
Pump(6000)
QuietMsgs = 1
Pump(14000)
Note("Nachrichten insgesamt=" + MsgCount)
StopNetGame
Note("ende")
End
