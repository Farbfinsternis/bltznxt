; Client nur IPv6: findet das Spiel nur ueber Feld 6
Include "../common.bb"
NetGameServer "http://127.0.0.1:28790/games"
Print "found " + FindNetGames(500)
Print "ip " + NetGameIP(1)
Print "join " + JoinNetGame(NetGameName(1), NetGameIP(1))
While RecvNetMsg() : Print "early " + Describe() : Wend
c = CreateNetPlayer("C6")
SendNetMsg 40, "hi", c, IdOf("H")
Delay 300
StopNetGame
Print "stopped"
