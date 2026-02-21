; Test Type Inference
x = 1.5
If x = 1.5 Then Print "Success: x is Float" Else Print "Fail: x is Int (" + x + ")"

y = 5
If y = 5 Then Print "Success: y is Int" Else Print "Fail: y is " + y

; Verify existing behavior isn't broken
Local z%
z = 1.5
If z = 1 Then Print "Success: z% is Int" Else Print "Fail: z% is " + z
End
