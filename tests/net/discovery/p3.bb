; Abgelehnte Beitritte und belegter Port, Gruende auf stderr
Include "../common.bb"
NetGamePort 28765
NetGameApp "other"
Print "app " + JoinNetGame("", Lo())
NetGameApp ""
Print "name " + JoinNetGame("wrong", Lo())
Print "port " + HostNetGame("second")
Print "search " + JoinNetGame("nope", "")
Print "unknown " + JoinNetGame("", "no.such.host.invalid")
Print "bracket " + JoinNetGame("", "[" + Lo())
Print "case " + JoinNetGame("NET_DISCOVERY_P1", LoUrl() + ":28765")
StopNetGame
Print "stopped"
