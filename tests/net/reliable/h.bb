; Empfaenger bei 20 % Paketverlust: 10 000 zuverlaessige Nachrichten in
; Reihenfolge und 1 MB am Stueck
Include "../common.bb"
NetGameApp "reliable"
NetGamePort 28762
Print "host " + HostNetGame("reliable")
h = CreateNetPlayer("H")
big$ = "0123456789abcdef"
For i = 1 To 16 : big = big + big : Next
n = 0 : bad = 0 : unrel = 0
StartClock(50000)
done = False
While (Not done) And (Not TimeUp())
	While RecvNetMsg()
		Select NetMsgType()
		Case 1
			If Int(NetMsgData()) = n + 1 Then n = n + 1 Else bad = bad + 1
		Case 2
			If NetMsgData() = big Then Print "big " + Len(NetMsgData()) + " ok" Else Print "big wrong " + Len(NetMsgData())
		Case 3
			unrel = unrel + 1
		Case 4
			Print "in order " + n + " bad " + bad
			Print "unreliable some " + (unrel > 0) + " not all " + (unrel < 200)
			SendNetMsg 5, "ok", h, NetMsgFrom()
		Case 101
			done = True
		End Select
	Wend
	Delay 1
Wend
StopNetGame
Print "stopped"
