; Milestone 22 — Timer Objects

; CreateTimer(20) → ticks every 50 ms
Local t = CreateTimer(20)

; First WaitTimer: returns >= 1 tick
Local ticks% = WaitTimer(t)
If ticks > 0 Then
    Print "WaitTimer OK"
Else
    Print "WaitTimer FAIL"
EndIf

; Second WaitTimer after busy delay: may return > 1 if ticks were missed
; Just check it returns > 0 either way
Delay 110   ; ~2 ticks will have expired
Local ticks2% = WaitTimer(t)
If ticks2 > 0 Then
    Print "WaitTimer catchup OK"
Else
    Print "WaitTimer catchup FAIL"
EndIf

; FreeTimer should not crash
FreeTimer t
Print "FreeTimer OK"

Print "DONE"
