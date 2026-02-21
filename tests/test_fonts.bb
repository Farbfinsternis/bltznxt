
; Test Font System
Graphics 800, 600, 0, 2

; Use a standard Windows font path for testing
font_path$ = "C:/Windows/Fonts/arial.ttf"

f1 = LoadFont(font_path, 24)
f2 = LoadFont(font_path, 48)

If Not f1 Then Print "Failed to load Font 1: " + font_path
If Not f2 Then Print "Failed to load Font 2: " + font_path

SetFont f1
While Not KeyHit(1)
    Cls
    
    Color 255, 255, 255
    SetFont f1
    Text 400, 100, "Arial 24 - Top-Left (0,0)", 0, 0
    Text 400, 150, "Arial 24 - Centered (1,1)", 1, 1
    
    Color 255, 255, 0
    SetFont f2
    Text 400, 300, "Arial 48 - Big Text", 1, 1
    
    SetFont f1
    Color 0, 255, 255
    Text 10, 500, "StringWidth: " + StringWidth("Arial 48 - Big Text")
    Text 10, 520, "StringHeight: " + StringHeight("Arial 48 - Big Text")
    
    Color 0, 255, 0
    Text MouseX(), MouseY(), "Floating Text", 1, 1
    
    Flip
Wend

FreeFont f1
FreeFont f2
End
