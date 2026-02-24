; Simple Blitz2D Example
Graphics 640,480,0,1
SetBuffer BackBuffer()
AppTitle "Blitz2D Basic Example"

; Load a player image
player = LoadImage("ship.png")
MidHandle player

x = 320
y = 240

PlayMusic("Sternenfeuer.ogg")

; Main Game Loop
While Not KeyHit(1)
    Cls ; Clear screen

    ; Input handling
    If KeyDown(203) Then x = x - 5
    If KeyDown(205) Then x = x + 5
    
    ; Draw player
    DrawImage player, x, y
    
    Flip ; Swap buffers
Wend
End
