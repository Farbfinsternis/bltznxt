
; Test compatibility for new transpiler features
DebugLog "=== Transpiler Feature Test ==="

; --- Test Include ---
DebugLog "Testing Include..."
Include "test_include.bb"
IncludedFunction()

; --- Test Select/Case ---
DebugLog "Testing Select/Case..."
Local i = 2
Select i
	Case 1
		DebugLog "  [Select] Case 1 (Failed)"
	Case 2
		DebugLog "  [Select] Case 2 (Success)"
	Default
		DebugLog "  [Select] Default (Failed)"
End Select

; Multiple expressions
Local c = 5
Select c
	Case 1, 2, 3
		DebugLog "  [Select] Case 1,2,3 (Failed)"
	Case 4, 5, 6
		DebugLog "  [Select] Case 4,5,6 (Success)"
End Select

; --- Test Repeat/Until ---
DebugLog "Testing Repeat/Until..."
Local counter = 0
Repeat
	counter = counter + 1
	DebugLog "  [Repeat] Loop " + counter
Until counter >= 3
DebugLog "  [Repeat] Finished"

; --- Test Gosub ---
DebugLog "Testing Gosub..."
Gosub MySubroutine
DebugLog "  [Gosub] Returned from subroutine (Success)"

End

.MySubroutine
	DebugLog "  [Gosub] Inside subroutine"
	Return

