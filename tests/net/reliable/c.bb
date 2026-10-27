; Sender bei 20 % Paketverlust
Include "../common.bb"
NetGameApp "reliable"
NetGamePort 28762
Print "join " + JoinNetGame("reliable", "127.0.0.1")
While RecvNetMsg() : Describe() : Wend
c = CreateNetPlayer("C")
hid = IdOf("H")
For i = 1 To 5000 : SendNetMsg 1, Str(i), c, hid : Next
big$ = "0123456789abcdef"
For i = 1 To 16 : big = big + big : Next
SendNetMsg 2, big, c, hid
For i = 5001 To 10000 : SendNetMsg 1, Str(i), c, hid : Next
For i = 1 To 200 : SendNetMsg 3, "u", c, hid, 0 : Next
SendNetMsg 4, "done", c, hid
StartClock(50000)
done = False
While (Not done) And (Not TimeUp())
	While RecvNetMsg()
		If NetMsgType() = 5 Then Print "acked " + NetMsgData() : done = True
	Wend
	Delay 1
Wend
StopNetGame
Print "stopped"
