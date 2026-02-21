DebugLog "--- Control Flow Test ---"

; --- Select Case ---
k = 2
Select k
    Case 1
        DebugLog "Select k=1"
    Case 2, 3
        DebugLog "Select k is 2 or 3"
    Default
        DebugLog "Select k is something else"
End Select

; --- While / Wend ---
count = 0
While count < 3
    count = count + 1
    DebugLog "While count: " + count
Wend

; --- Repeat / Until ---
r = 0
Repeat
    r = r + 1
    DebugLog "Repeat count: " + r
Until r >= 3

; --- Repeat / Forever / Exit ---
f = 0
Repeat
    f = f + 1
    DebugLog "Forever count: " + f
    If f = 2 Then Exit
Forever

DebugLog "--- End Control Flow Test ---"
