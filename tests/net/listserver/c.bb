; Sucht nur beim Listen-Server (BLTZNXT_NET_LAN=0) und tritt bei
Include "../common.bb"
NetGameServer "http://127.0.0.1:28790/games"
n = FindNetGames(500)
Print "found " + n
Print NetGameName(1) + " players " + NetGamePlayers(1) + " lan " + NetGameLAN(1) + " ip " + NetGameIP(1)
Print "join " + JoinNetGame(NetGameName(1), NetGameIP(1))
While RecvNetMsg() : Print "early " + Describe() : Wend
c = CreateNetPlayer("C")
Delay 300
StopNetGame
Print "stopped"
