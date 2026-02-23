; Milestone 28 — Bank Allocation

; ----- CreateBank / BankSize -----
Local b = CreateBank(16)
If BankSize(b) = 16 Then
    Print "CreateBank OK"
Else
    Print "CreateBank FAIL"
EndIf

; ----- ResizeBank -----
ResizeBank b, 32
If BankSize(b) = 32 Then
    Print "ResizeBank OK"
Else
    Print "ResizeBank FAIL"
EndIf

; ----- CopyBank -----
Local b2 = CreateBank(8)
CopyBank b, 0, b2, 0, 8
If BankSize(b2) = 8 Then
    Print "CopyBank OK"
Else
    Print "CopyBank FAIL"
EndIf

; ----- FreeBank -----
FreeBank b2
FreeBank b
Print "FreeBank OK"

Print "DONE"
