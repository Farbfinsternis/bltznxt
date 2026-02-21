Graphics3D 640,480,0,2
DebugLog "Press Arrows - ESC to exit"
While Not KeyDown(1)
    If KeyDown(200) Then DebugLog "UP pressed"
    If KeyDown(208) Then DebugLog "DOWN pressed"
    If KeyDown(203) Then DebugLog "LEFT pressed"
    If KeyDown(205) Then DebugLog "RIGHT pressed"
    Flip
Wend
End
