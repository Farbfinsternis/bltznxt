; Milestone 21 — Time Functions

; ----- MilliSecs -----
Local t1% = MilliSecs()
Delay 50
Local t2% = MilliSecs()

If t2 > t1 Then
    Print "MilliSecs OK"
Else
    Print "MilliSecs FAIL"
EndIf

; ----- CurrentDate -----
; Format: "DD Mon YYYY" → always 11 chars
Local d$ = CurrentDate()
If Len(d) = 11 Then
    Print "CurrentDate OK"
Else
    Print "CurrentDate FAIL"
EndIf

; ----- CurrentTime -----
; Format: "HH:MM:SS" → always 8 chars
Local t$ = CurrentTime()
If Len(t) = 8 Then
    Print "CurrentTime OK"
Else
    Print "CurrentTime FAIL"
EndIf

Print "DONE"
