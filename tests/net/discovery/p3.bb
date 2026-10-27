; Abgelehnte Beitritte und belegter Port, Gruende auf stderr
NetGamePort 28765
NetGameApp "other"
Print "app " + JoinNetGame("", "127.0.0.1")
NetGameApp ""
Print "name " + JoinNetGame("wrong", "127.0.0.1")
Print "port " + HostNetGame("second")
Print "search " + JoinNetGame("nope", "")
Print "unknown " + JoinNetGame("", "no.such.host.invalid")
Print "case " + JoinNetGame("NET_DISCOVERY_P1", "127.0.0.1")
StopNetGame
Print "stopped"
