; Client nur mit IPv6 findet das Spiel ueber den Listen-Server; Status und Log
Include "../common.bb"
Include "logcheck.bb"
NetGameLog "bin/net_netlog/c.log"
NetGameServer "http://127.0.0.1:28799/games"
Print "status before " + NetGameServerStatus()
n = FindNetGames(300)
Print "dead server " + n + " status " + NetGameServerStatus()
NetGameServer "http://127.0.0.1:28790/games"
n = FindNetGames(500)
Print "found " + n + " status " + NetGameServerStatus()
Print "join " + JoinNetGame(NetGameName(1), NetGameIP(1))
c = CreateNetPlayer("Clienta")
Delay 300
StopNetGame
Delay 100
LogCheck("bin/net_netlog/c.log", "client sockets: IPv4 no, IPv6 yes (bound by BIND)|this computer: IPv4 none, IPv6 loopback|list server: no list (HTTP 0, no answer)|list server: 1 game(s), 1 with IPv4, 1 with IPv6|search: 1 game(s), 0 in the LAN, 1 from the list server|join: trying 2 addresses (IPv6, IPv4), game from the list server|knock: accepted by the list server|join: joined over IPv6|game stopped")
