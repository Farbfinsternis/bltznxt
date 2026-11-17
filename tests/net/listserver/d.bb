; Nach dem Ende des Hosts ist die Liste leer
Include "../common.bb"
NetGameServer "http://" + LoUrl() + ":28790/games"
Print "found " + FindNetGames(300)
