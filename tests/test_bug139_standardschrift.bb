; BUG-139 - Standardschrift und LoadFont("courier") wie im Original.
;
; Die Standardschrift von Blitz3D ist die GDI-Bitmapschrift Courier in 12
; (13 Pixel hoch, 8 breit); wir hatten eine eingebaute 8x8-Schrift. Courier
; gibt es nur in 13, 16 und 20 Pixeln, auch ganzzahlig vergroessert; GDI nimmt
; die groesste Groesse, die nicht ueber der verlangten Hoehe liegt.
; Erwartung: Ausgabe von Blitz3D 11.8 (build/bug139_20261026).
Graphics 320,200,0,2
SetBuffer BackBuffer()
Print FontHeight()+" "+FontWidth()+" "+StringWidth("Hello World")+" "+StringHeight("x")
Cls
Color 255,255,255
Text 0,0,"Hg"
Text 0,13,"Mi"
LockBuffer BackBuffer()
n=0 : h=0 : x1=-1 : y1=-1
For y=0 To 30
	For x=0 To 40
		If (ReadPixelFast(x,y) And $FFFFFF)<>0 Then
			n=n+1 : h=(h*31+x*977+y*131) And $FFFFFF
			If x>x1 Then x1=x
			If y>y1 Then y1=y
		EndIf
	Next
Next
UnlockBuffer BackBuffer()
Print "Text n="+n+" x1="+x1+" y1="+y1+" h="+h
Restore groessen
Repeat
	Read g,fett
	If g=0 Then Exit
	f=LoadFont("courier",g,fett)
	SetFont f
	Print g+" "+fett+": "+FontHeight()+" "+FontWidth()+" "+StringWidth("Hello World")
	FreeFont f
Forever
.groessen
Data 6,0, 12,0, 15,0, 16,0, 19,0, 20,0, 24,0, 26,0, 36,0, 40,0, 48,0, 12,1, 16,1, 0,0
; Bildpuffer, auch vergroessert (26 = 2 x 13)
SetBuffer BackBuffer()
For g=12 To 26 Step 14
	img=CreateImage(80,40)
	SetBuffer ImageBuffer(img)
	f=LoadFont("courier",g)
	SetFont f
	Color 255,255,255
	Text 0,0,"Hg"
	LockBuffer ImageBuffer(img)
	n=0 : h=0
	For y=0 To 39
		For x=0 To 79
			If (ReadPixelFast(x,y) And $FFFFFF)<>0 Then n=n+1 : h=(h*31+x*977+y*131) And $FFFFFF
		Next
	Next
	UnlockBuffer ImageBuffer(img)
	Print g+": n="+n+" h="+h
	SetBuffer BackBuffer()
	FreeImage img
Next
