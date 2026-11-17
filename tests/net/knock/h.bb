; Host nur mit IPv6 hinter einer nachgebauten Router-Firewall
; (BLTZNXT_NET_FIREWALL): Pakete von Fremden kommen erst durch, wenn er
; ihnen selbst etwas geschickt hat. Er fragt den Listen-Server nach Klopfern.
Include "../common.bb"
NetGameServer "http://127.0.0.1:28790/games"
NetGamePort 28771
Print "host " + HostNetGame("knock")
h = CreateNetPlayer("H")
StartClock(30000)
done = False
While (Not done) And (Not TimeUp())
	While RecvNetMsg()
		Print Describe()
		If NetMsgType() = 101 And NameOf(NetMsgFrom()) = "K" Then done = True
	Wend
	Delay 2
Wend
StopNetGame
Print "stopped"
