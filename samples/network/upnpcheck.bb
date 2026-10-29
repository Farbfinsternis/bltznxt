; Prueft, ob der Router den Spielport von selbst weiterleitet (UPnP).
; Hostet ein Spiel, wartet auf die Antwort des Routers, haelt die
; Weiterleitung 60 Sekunden (Zeit, sie in der Router-Oberflaeche zu sehen)
; und gibt sie dann zurueck. Gruende fuer ein Scheitern stehen auf stderr.

Print "Hosting on UDP port 2350 ..."
If HostNetGame("upnpcheck") = 0
	Print "Could not host (port in use?)"
	End
EndIf

t = MilliSecs()
While NetGameForwarded() = -1 And MilliSecs() - t < 10000
	Delay 50
Wend

If NetGameForwarded() = 1
	Print "The router forwards UDP port 2350 to this computer (" + (MilliSecs() - t) + " ms)."
Else
	Print "No forwarding. See the reason above (stderr)."
EndIf

Print "Keeping the game open for 60 seconds ..."
Delay 60000
StopNetGame
Print "Stopped, the forwarding is removed."
