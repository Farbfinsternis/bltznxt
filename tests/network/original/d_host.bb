; Szenario D, Host: Nachrichtentypen, Groessen, viele Nachrichten auf einmal (Fragen 5, 7)
Include "netlog.bb"
LogInit()
r = HostNetGame("messd")
Note("HostNetGame=" + r)
If r <> 2 Then End
h = CreateNetPlayer("HD")
Note("CreateNetPlayer HD=" + h)
If WaitJoin(20000) = 0
	Note("kein Beitritt")
	End
EndIf
Pump(500)
; Typen ausserhalb 1..99
For t = -1 To 300
	If t = -1 Or t = 0 Or t = 1 Or t = 99 Or t = 100 Or t = 101 Or t = 102 Or t = 150 Or t = 200 Or t = 255 Or t = 256 Or t = 300
		SendNetMsg t, "typ " + t, h, 0, 1
		Note("SendNetMsg typ " + t)
	EndIf
Next
Pump(1000)
; Groessen, zuverlaessig und unzuverlaessig
Restore Groessen
For i = 1 To 7
	Read size
	SendNetMsg 40, String$("x", size), h, 0, 1
	Note("SendNetMsg 40 zuverlaessig len=" + size)
	SendNetMsg 41, String$("y", size), h, 0, 0
	Note("SendNetMsg 41 unzuverlaessig len=" + size)
	Pump(300)
Next
Pump(1000)
; viele auf einmal, ohne Pause
For i = 1 To 300
	SendNetMsg 50, "u" + i, h, 0, 0
Next
Note("300 x unzuverlaessig gesendet")
For i = 1 To 300
	SendNetMsg 51, "z" + i, h, 0, 1
Next
Note("300 x zuverlaessig gesendet")
Pump(8000)
StopNetGame
Note("ende")
End

.Groessen
Data 100, 1000, 10000, 60000, 100000, 500000, 1000000
