; BUG-221 - Nach Flip bleibt die Flaeche im Fenster erhalten.
;
; Im Fenster sind Front- und Backbuffer im Original dieselbe Flaeche; Flip
; zeigt sie nur. Programme, die ohne Cls weiterzeichnen (Spuren, Malen,
; einmal geschriebener Text), verlassen sich darauf. Bei BLTZNXT war der
; Backbuffer nach jedem Flip undefiniert.
; Erwartung: Ausgabe von Blitz3D 11.8.
Graphics 200,150,0,2
SetBuffer BackBuffer()
Color 255,0,0 : Rect 10,10,20,20
Flip
Print Hex(ReadPixel(20,20) And $FFFFFF)
Color 0,255,0 : Rect 40,10,20,20
Flip : Flip
Print Hex(ReadPixel(20,20) And $FFFFFF)+" "+Hex(ReadPixel(50,20) And $FFFFFF)
For i=0 To 9 : Plot 100+i,100 : Flip : Next
n=0
For i=0 To 9
	If (ReadPixel(100+i,100) And $FFFFFF)=$00FF00 Then n=n+1
Next
Print "spur "+n
SetBuffer FrontBuffer()
Color 0,0,255 : Rect 70,10,20,20
Flip
Print Hex(ReadPixel(80,20,BackBuffer()) And $FFFFFF)+" "+Hex(ReadPixel(20,20,FrontBuffer()) And $FFFFFF)
Cls : Flip
Print Hex(ReadPixel(20,20) And $FFFFFF)
Print "DONE"
