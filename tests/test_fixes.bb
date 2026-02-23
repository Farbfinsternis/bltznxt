; Test: Bug-fix verification for BlitzNext

; --- Bug 1: Assignment ---
Local x% = 10
x% = x% + 5
Print "x should be 15: " + Str(x%)

; --- Bug 2: AND / OR / NOT (logical operators) ---
Local a% = 1
Local b% = 0
If a% And Not b% Then
    Print "AND/NOT: OK"
EndIf

If a% Or b% Then
    Print "OR: OK"
EndIf

; --- Bug 4: ElseIf ---
Local n% = 2
If n% = 1 Then
    Print "n=1"
ElseIf n% = 2 Then
    Print "ElseIf: OK (n=2)"
ElseIf n% = 3 Then
    Print "n=3"
Else
    Print "other"
EndIf

; --- Bug 5: For with negative Step ---
Print "Countdown:"
For i% = 3 To 1 Step -1
    Print Str(i%)
Next i%

; --- Bug 12: For without Step (still works) ---
Local sum% = 0
For j% = 1 To 5
    sum% = sum% + j%
Next
Print "Sum 1-5 should be 15: " + Str(sum%)

; --- Bug 6: Type hint ! (float) ---
Local f! = 3.14
Print "Pi approx: " + Str(f!)

; --- Function declaration and call ---
Function Double%(val%)
    Return val% * 2
End Function

Local result% = Double%(7)
Print "Double(7) should be 14: " + Str(result%)

; --- Single-line IF ---
If result% = 14 Then Print "Single-line If: OK"

WaitKey
