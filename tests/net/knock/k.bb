; Findet das Spiel beim Listen-Server, klopft dort an und tritt bei, sobald
; der Host nachgefragt und seine Firewall geoeffnet hat
Include "../common.bb"
NetGameServer "http://127.0.0.1:28790/games"
Print "found " + FindNetGames(500)
Print "ip " + NetGameIP(1)
Print "join " + JoinNetGame(NetGameName(1), NetGameIP(1))
While RecvNetMsg() : Print "early " + Describe() : Wend
k = CreateNetPlayer("K")
SendNetMsg 40, "hi", k, IdOf("H")
Delay 300
StopNetGame
Print "stopped"
