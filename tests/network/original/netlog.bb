; Gemeinsamer Teil der DirectPlay-Messprogramme (Arbeitspaket 1, NETWORK.md).
;
; Aufruf jedes Messprogramms:  prog.exe <protokolldatei> [argument]
; Jede Zeile im Protokoll beginnt mit den Millisekunden seit Programmstart.
; Die Datei wird je Zeile geoeffnet und geschlossen, damit nach einem Absturz
; oder Abbruch alles bis zur letzten Zeile erhalten bleibt.

Global LogPath$, Arg$, T0
Global FirstRemote     ; erster fremder Spieler, von dem eine 100 kam
Global LastJoined      ; zuletzt per 100 gemeldeter Spieler
Global Got102          ; Anzahl empfangener 102
Global MsgCount        ; Anzahl empfangener Nachrichten
Global QuietMsgs       ; 1 = Nutzernachrichten nur zaehlen, nicht einzeln protokollieren

Function LogInit()
	c$ = Trim(CommandLine$())
	p = Instr(c$, " ")
	If p
		LogPath$ = Left(c$, p - 1)
		Arg$ = Trim(Mid(c$, p + 1))
	Else
		LogPath$ = c$
	EndIf
	f = WriteFile(LogPath$)
	CloseFile f
	T0 = MilliSecs()
	Note("start arg=[" + Arg$ + "]")
End Function

Function Note(s$)
	f = OpenFile(LogPath$)
	SeekFile f, FileSize(LogPath$)
	WriteLine f, (MilliSecs() - T0) + " " + s$
	CloseFile f
End Function

; Holt alle wartenden Nachrichten und protokolliert sie. Die Rohfelder stehen
; in einer eigenen Zeile vor NetPlayerName/NetPlayerLocal, falls diese bei
; From = 0 abstuerzen.
Function LogMsgs()
	While RecvNetMsg()
		ty = NetMsgType()
		fr = NetMsgFrom()
		tt = NetMsgTo()
		d$ = NetMsgData$()
		MsgCount = MsgCount + 1
		If ty = 100
			If FirstRemote = 0 Then FirstRemote = fr
			LastJoined = fr
		EndIf
		If ty = 102 Then Got102 = Got102 + 1
		If QuietMsgs = 0 Or ty >= 100
			s$ = "msg type=" + ty + " from=" + fr + " to=" + tt + " len=" + Len(d$)
			If Len(d$) <= 40 Then s$ = s$ + " data=[" + d$ + "]"
			Note(s$)
			If fr <> 0 Then Note("    name=[" + NetPlayerName$(fr) + "] local=" + NetPlayerLocal(fr))
		Else
			Note("msg type=" + ty + " len=" + Len(d$) + " data=[" + Left(d$, 12) + "]")
		EndIf
	Wend
End Function

; Empfaengt ms Millisekunden lang.
Function Pump(ms)
	t = MilliSecs()
	Repeat
		LogMsgs()
		Delay 5
	Until MilliSecs() - t >= ms
End Function

; Empfaengt, bis eine 100 kommt oder ms vergangen sind; 1 bei Erfolg.
Function WaitJoin(ms)
	t = MilliSecs()
	prev = LastJoined
	Repeat
		LogMsgs()
		If LastJoined <> prev Then Return 1
		Delay 5
	Until MilliSecs() - t >= ms
	Return 0
End Function

; Teilt "a|b" und liefert den Teil index (0 oder 1).
Function Part$(s$, index)
	p = Instr(s$, "|")
	If p = 0
		If index = 0 Then Return s$ Else Return ""
	EndIf
	If index = 0 Then Return Left(s$, p - 1)
	Return Mid(s$, p + 1)
End Function
