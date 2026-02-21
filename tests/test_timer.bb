Graphics 640,480,0,2
timer = CreateTimer(1) ; 1 FPS
Print "Testing 3 ticks at 1 FPS..."
For i = 1 To 3
    WaitTimer(timer)
    Print "Tick " + i + " at " + MilliSecs()
Next
FreeTimer(timer)
Print "Done."
Delay 1000
End
