; StartNetGame mit BLTZNXT_NET_JOIN: tritt genau dort bei
Include "../common.bb"
Print "start " + StartNetGame()
While RecvNetMsg() : Print "early " + Describe() : Wend
p4 = CreateNetPlayer("P4")
Delay 300
StopNetGame
Print "stopped"
