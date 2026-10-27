; Gemeinsame Hilfen der Mehrprozess-Tests (tests/net/run_net_tests.sh).
; Spieler werden mit ihrem Namen gemerkt, damit eine 101 noch sagt, wer ging.

Type KnownPlayer
	Field id, name$
End Type

Global Deadline

Function StartClock(ms)
	Deadline = MilliSecs() + ms
End Function

Function TimeUp()
	If MilliSecs() > Deadline
		Print "timeout"
		Return True
	EndIf
	Return False
End Function

Function Remember(id)
	k.KnownPlayer = New KnownPlayer
	k\id = id
	k\name = NetPlayerName(id)
End Function

Function NameOf$(id)
	For k.KnownPlayer = Each KnownPlayer
		If k\id = id Then Return k\name
	Next
	Return NetPlayerName(id)
End Function

Function IdOf(name$)
	For k.KnownPlayer = Each KnownPlayer
		If k\name = name Then Return k\id
	Next
	Return 0
End Function

; Die zuletzt geholte Nachricht als Text
Function Describe$()
	t = NetMsgType()
	Select t
	Case 100
		Remember(NetMsgFrom())
		Return "100 " + NameOf(NetMsgFrom())
	Case 101
		Return "101 " + NameOf(NetMsgFrom())
	Case 102
		Return "102 from " + NetMsgFrom()
	Case 200
		Return "200"
	End Select
	Return t + " " + NameOf(NetMsgFrom()) + "->" + NameOf(NetMsgTo()) + " [" + NetMsgData() + "]"
End Function
