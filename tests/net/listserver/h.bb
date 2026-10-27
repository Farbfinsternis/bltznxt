; Host meldet sich beim Listen-Server an und beim Ende wieder ab
Include "../common.bb"
NetGameServer "http://127.0.0.1:28790/games"
NetGamePort 28766
Print "host " + HostNetGame("listed")
h = CreateNetPlayer("H")
StartClock(20000)
done = False
While (Not done) And (Not TimeUp())
	While RecvNetMsg()
		Print Describe()
		If NetMsgType() = 101 Then done = True
	Wend
	Delay 2
Wend
StopNetGame
Print "stopped"
