; Milestone 23 — System / Process Functions

; ----- AppTitle (no-op in console mode) -----
AppTitle "BlitzNext Test"
Print "AppTitle OK"

; ----- CommandLine (no extra args in test runner) -----
Local cl$ = CommandLine()
If Len(cl) = 0 Then
    Print "CommandLine OK"
Else
    Print "CommandLine FAIL"
EndIf

; ----- SetEnv / GetEnv round-trip -----
SetEnv "BLITZNEXT_TEST_KEY", "hello42"
Local v$ = GetEnv("BLITZNEXT_TEST_KEY")
If v = "hello42" Then
    Print "SetEnv/GetEnv OK"
Else
    Print "SetEnv/GetEnv FAIL"
EndIf

; ----- GetEnv for non-existent key → "" -----
Local nope$ = GetEnv("BLITZNEXT_NONEXISTENT_XYZ_99")
If Len(nope) = 0 Then
    Print "GetEnv missing OK"
Else
    Print "GetEnv missing FAIL"
EndIf

; ----- SystemProperty stub -----
Local sp$ = SystemProperty("unknown")
If Len(sp) = 0 Then
    Print "SystemProperty OK"
Else
    Print "SystemProperty FAIL"
EndIf

Print "DONE"
