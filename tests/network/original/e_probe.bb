; Szenario E: Fehlerfaelle, ein Fall je Lauf, weil manche abstuerzen koennen
; (Fragen 5, 6). Argument = Name des Falls.
Include "netlog.bb"
LogInit()
Select Arg$
Case "recv_ohne_spiel"
	Note("RecvNetMsg ohne Spiel=" + RecvNetMsg())
Case "send_ohne_spiel"
	SendNetMsg 1, "x", 12345, 0, 1
	Note("SendNetMsg ohne Spiel ueberstanden")
Case "player_ohne_spiel"
	Note("CreateNetPlayer ohne Spiel=" + CreateNetPlayer("X"))
Default
	r = HostNetGame("messe")
	Note("HostNetGame=" + r)
	Select Arg$
	Case "send_ohne_spieler"
		SendNetMsg 1, "x", 12345, 0, 1
		Note("SendNetMsg ohne Spieler ueberstanden")
	Case "send_fremde_id"
		p = CreateNetPlayer("E")
		SendNetMsg 1, "x", 12345, 0, 1
		Note("SendNetMsg mit fremder from-ID ueberstanden")
		SendNetMsg 1, "x", p, 12345, 1
		Note("SendNetMsg an unbekannte to-ID ueberstanden")
	Case "name_unbekannt"
		p = CreateNetPlayer("E")
		Note("NetPlayerName(12345)=[" + NetPlayerName$(12345) + "]")
		Note("NetPlayerLocal(12345)=" + NetPlayerLocal(12345))
		Note("NetPlayerName(0)=[" + NetPlayerName$(0) + "]")
		Note("NetPlayerLocal(0)=" + NetPlayerLocal(0))
	Case "delete_unbekannt"
		p = CreateNetPlayer("E")
		DeleteNetPlayer 12345
		Note("DeleteNetPlayer(12345) ueberstanden")
	Case "msg_ohne_recv"
		p = CreateNetPlayer("E")
		Note("NetMsgType ohne RecvNetMsg=" + NetMsgType() + " from=" + NetMsgFrom() + " to=" + NetMsgTo() + " data=[" + NetMsgData$() + "]")
	Case "host_zweimal"
		Note("HostNetGame erneut=" + HostNetGame("messe2"))
	End Select
	StopNetGame
	Note("StopNetGame")
	Note("StopNetGame zweimal vor")
	StopNetGame
	Note("RecvNetMsg nach Stop=" + RecvNetMsg())
End Select
Note("ende")
End
