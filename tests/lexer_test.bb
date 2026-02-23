; BlitzNext Lexer Test
Graphics 640,480,32,2
SetBuffer BackBuffer()

Local x = 10
Local y# = 20.5
Local s$ = "Hello BlitzNext"

While Not KeyHit(1)
    Cls
    Text x,y,s
    Flip
Wend

End
