; Test String Comparisons
s1$ = "Hello"
s2$ = "HELLO"

Print "Testing s1=" + s1 + " s2=" + s2

If s1 = s2 Then
    Print "Success: 'Hello' = 'HELLO'"
Else
    Print "Fail: 'Hello' <> 'HELLO' (Case Sensitive?)"
EndIf

If s1$ = "hello" Then
    Print "Success: 'Hello' = 'hello'"
Else
    Print "Fail: 'Hello' <> 'hello'"
EndIf

If s1 <> "World" Then
    Print "Success: 'Hello' <> 'World'"
Else
    Print "Fail: 'Hello' = 'World'"
EndIf
