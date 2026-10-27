; Ein zweites Spiel neben dem laufenden ist ein Laufzeitfehler wie im Original
; ("Multiplayer game already started", gemessen 2026-10-27).
NetGamePort 28769
Print HostNetGame("one")
p = CreateNetPlayer("P")
Print NetPlayerName(p) + " " + NetPlayerLocal(p) + " " + NetGameHost()
SendNetMsg 3, "to myself", p, p
Print RecvNetMsg() + " " + NetMsgType() + " " + NetMsgData()
Print RecvNetMsg()
Print JoinNetGame("one", "127.0.0.1")
Print "not reached"
