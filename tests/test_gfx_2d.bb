
; Test 2D Graphics and Input
Graphics 800, 600, 0, 2

Print "Testing Primitives..."
Color 255, 0, 0
Rect 50, 50, 100, 100, 1
Color 0, 255, 0
Rect 200, 50, 100, 100, 0
Color 0, 0, 255
Line 50, 200, 300, 200
Color 255, 255, 0
Oval 50, 300, 100, 50, 1

Print "Move Mouse. Left=Red, Right=Blue, Middle=Green Box."
Print "Press SPACE to Exit."

FlushMouse
While Not KeyHit(57)
    Cls
    
    mx = MouseX()
    my = MouseY()
    
    ; Draw crosshair
    Color 255, 255, 255
    Line mx-10, my, mx+10, my
    Line mx, my-10, mx, my+10
    
    ; Status text (using text for now)
    Color 200, 200, 200
    Text 10, 500, "Mouse: " + mx + ", " + my
    
    If MouseDown(1) Then Color 255, 0, 0 : Rect 10, 10, 50, 50, 1
    If MouseDown(2) Then Color 0, 0, 255 : Rect 70, 10, 50, 50, 1
    If MouseDown(3) Then Color 0, 255, 0 : Rect 130, 10, 50, 50, 1
    
    Flip
Wend
End
