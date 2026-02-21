; Test Multi-Line If / ElseIf
x = 10
If x < 5 Then
    Print "x < 5"
ElseIf x < 15 Then
    Print "x < 15"
    If x > 8 Then
        Print "Nested: x > 8"
    EndIf
Else
    Print "x >= 15"
EndIf

; Test Single-Line If with Colons
y = 20
If y = 20 Then Print "y is 20" : Print "Correct" Else Print "y is not 20" : Print "Incorrect"
If y = 0 Then Print "y is 0" Else Print "y is not 0" : Print "Also Correct"

End
