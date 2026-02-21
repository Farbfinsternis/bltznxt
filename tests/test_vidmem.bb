; Test Video Memory Stats
Graphics 800,600,0,2

total = TotalVidMem()
avail = AvailVidMem()

Print "Video Memory Statistics:"
Print "Total: " + (total / 1024 / 1024) + " MB"
Print "Avail: " + (avail / 1024 / 1024) + " MB"

If total < 1024*1024*1024 Then 
    Print "Warning: Reported Total VidMem seems low or detection failed (using fallback/clamped)."
EndIf

Print ""
Print "Press any key to exit."
WaitKey()
End
