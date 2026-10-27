; Net-Befehle ohne Spiel (bb_net.h). Namen und Suche gehen ohne Spiel, Senden
; nicht: wie im Original "Multiplayer game not started" (gemessen 2026-10-27).
Print NetPlayerName(0)
Print NetPlayerName(12345)
Print NetPlayerLocal(12345)
Print NetMsgType() + " " + NetMsgFrom() + " " + NetMsgTo() + " [" + NetMsgData() + "]"
NetGamePort 28768
NetGameApp "errors"
Print FindNetGames(200)
Print "[" + NetGameName(1) + "] [" + NetGameIP(1) + "] " + NetGamePlayers(1) + " " + NetGameLAN(1)
Print NetGameHost()
StopNetGame
DeleteNetPlayer 5
SendNetMsg 1, "x", 1
Print "not reached"
