; Prueft ein Diagnose-Log: jede gesuchte Wendung kommt vor, keine Adresse
; dieses Rechners und kein Spielername steht darin
Function LogCheck(file$, want$)
	f = ReadFile(file)
	If f = 0 Then Print "log missing" : Return
	all$ = ""
	While Not Eof(f)
		all = all + ReadLine(f) + Chr(10)
	Wend
	CloseFile f
	; want$: Wendungen, durch | getrennt
	rest$ = want + "|"
	While rest <> ""
		p = Instr(rest, "|")
		w$ = Left(rest, p - 1)
		rest = Mid(rest, p + 1)
		If w <> "" Then
			If Instr(all, w) Then Print "found: " + w Else Print "MISSING: " + w
		EndIf
	Wend
	If Instr(all, "127.0.0.1") Or Instr(all, "::1") Then Print "ADDRESS IN LOG" Else Print "no address"
	If Instr(all, "Hosti") Or Instr(all, "Clienta") Or Instr(all, "lognet") Then Print "NAME IN LOG" Else Print "no name"
End Function
