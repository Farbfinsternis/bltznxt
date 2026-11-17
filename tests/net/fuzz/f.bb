; Tritt einem falschen Host bei, der kaputte Pakete schickt, ueberlebt das und
; tritt danach dem echten Host bei (der gerade selbst beschossen wird)
Include "../common.bb"
Print "fake join " + JoinNetGame("", LoUrl() + ":28781")
t = MilliSecs() + 3000
While MilliSecs() < t
	While RecvNetMsg() : Wend
	Delay 5
Wend
StopNetGame
Print "survived"
Print "join " + JoinNetGame("fuzz", LoUrl() + ":28780")
While RecvNetMsg() : d$ = Describe() : Wend
fp = CreateNetPlayer("F")
SendNetMsg 41, "F ok", fp, IdOf("H")
Delay 300
StopNetGame
Print "stopped"
