Graphics3D 800,600,32,2
SetBuffer BackBuffer()

; Test font loading (loading Arial from Windows Fonts)
arial24 = LoadFont("arial", 24)
arial24_bold = LoadFont("arial", 24, 1)

If Not arial24 Then RuntimeError "Failed to load Arial font!"

While Not KeyHit(1)
	Cls
	
	Color 255,255,255
	
	; Test SetFont and Text
	SetFont arial24
	Text 10, 10, "Arial 24 - Width: " + FontWidth() + " Height: " + FontHeight()
	Text 10, 40, "StringWidth('Hello'): " + StringWidth("Hello")
	
	SetFont arial24_bold
	Text 10, 80, "Arial 24 Bold - Width: " + FontWidth() + " Height: " + FontHeight()
	
	; Center test
	Text 400, 300, "CENTERED TEXT", 1, 1
	
	Flip
Wend
End
