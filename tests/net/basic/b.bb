; Zweiter Client: sieht alle drei Spieler beim Beitritt, loescht seinen Spieler
Include "../common.bb"
NetGameApp "basic"
NetGamePort 28761
Print "join " + JoinNetGame("basic", "127.0.0.1")
While RecvNetMsg()
	Print "early " + Describe()
Wend
b1 = CreateNetPlayer("B1") : Remember(b1)
SendNetMsg 20, "B1 to all", b1
StartClock(20000)
done = False
While (Not done) And (Not TimeUp())
	While RecvNetMsg()
		Print Describe()
		If NetMsgType() = 30
			DeleteNetPlayer b1
			Print "deleted " + NetPlayerName(b1) + " " + NetPlayerLocal(b1)
			done = True
		EndIf
	Wend
	Delay 2
Wend
StopNetGame
Print "stopped"
