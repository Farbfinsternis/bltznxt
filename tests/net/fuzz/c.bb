; Kommt nach dem Beschuss: der Host antwortet normal
Include "../common.bb"
Print "join " + JoinNetGame("fuzz", LoUrl() + ":28780")
While RecvNetMsg() : d$ = Describe() : Wend
cp = CreateNetPlayer("C")
SendNetMsg 40, "C ok", cp, IdOf("H")
Delay 300
StopNetGame
Print "stopped"
