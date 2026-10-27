; StartNetGame ohne laufendes Spiel: sucht, findet nichts, hostet
Include "../common.bb"
NetGamePort 28765
Print "start " + StartNetGame()
p1 = CreateNetPlayer("P1") : Remember(p1)
StartClock(30000)
done = False
While (Not done) And (Not TimeUp())
	While RecvNetMsg()
		Print Describe()
		If NetMsgType() = 101 And NameOf(NetMsgFrom()) = "P4" Then done = True
	Wend
	Delay 2
Wend
StopNetGame
Print "stopped"
