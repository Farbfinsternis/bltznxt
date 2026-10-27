; FindNetGames listet das Spiel, StartNetGame tritt bei
Include "../common.bb"
NetGamePort 28765
n = FindNetGames(600)
Print "found " + n
Print NetGameName(1) + " players " + NetGamePlayers(1) + " lan " + NetGameLAN(1) + " ip " + NetGameIP(1)
Print "beyond [" + NetGameName(2) + "] " + NetGamePlayers(0)
Print "start " + StartNetGame()
While RecvNetMsg() : Print "early " + Describe() : Wend
p2 = CreateNetPlayer("P2")
Delay 300
StopNetGame
Print "stopped"
