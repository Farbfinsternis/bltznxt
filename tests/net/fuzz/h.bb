; Host unter Beschuss (tests/net/fuzz.js): laeuft weiter und nimmt danach
; echte Programme auf. Gedruckt wird nur, was von F und C kommt.
Include "../common.bb"
NetGamePort 28780
Print "host " + HostNetGame("fuzz")
h = CreateNetPlayer("H")
StartClock(40000)
done = False
While (Not done) And (Not TimeUp())
	While RecvNetMsg()
		d$ = Describe()
		t = NetMsgType()
		n$ = NameOf(NetMsgFrom())
		If t = 40 Or t = 41 Or ((t = 100 Or t = 101) And (n = "F" Or n = "C")) Then Print d
		If t = 101 And n = "C" Then done = True
	Wend
	Delay 2
Wend
StopNetGame
Print "stopped"
