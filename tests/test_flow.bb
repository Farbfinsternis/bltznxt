
; Test Control Flow
Print "Testing Control Flow..."

; Select
val = 2
Select val
    Case 1
        Print "Select Failed (1)"
    Case 2, 3
        Print "Select OK (2 or 3)"
    Default
        Print "Select Failed (Default)"
End Select

; While
i = 0
While i < 3
    Print "While " + i
    i = i + 1
Wend

; Repeat
j = 0
Repeat
    Print "Repeat " + j
    j = j + 1
Until j = 3

; Gosub
Gosub MySub
Print "Gosub Return OK"

Print "Done."
WaitKey
End

.MySub
Print "Inside Sub"
Return
