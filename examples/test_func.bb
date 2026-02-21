Function Greet(name$="World", times=1)
    For i=1 To times
        DebugLog "Hello, " + name
    Next
End Function

DebugLog "--- Function Test ---"
Greet()
Greet("Blitz")
Greet("Next", 3)
DebugLog "--- End Function Test ---"
