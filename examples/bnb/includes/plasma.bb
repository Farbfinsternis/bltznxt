; Plasmaeffekt Initialisierungen
Global plasma_tex=CreateTexture(256,256)
Dim cosinus(640)
position = 0
For c = 0 To 640
 cosinus(c) = Cos((115*3.14159265358 * c) / 320) * 32 + 32
Next 
;Create some Color Arrays
Dim r(255) : Dim g(255) : Dim b(255) : Dim mrgb(255)
For i=0 To 63 : r(i)=i*4 : g(i)=0 : b(i)=255-(i*4) : Next
For i=0 To 63 : r(i+64)=255-(i*4) : g(i+64)=i*2 : b(i+64)=0 : Next
For i=0 To 63 : r(i+128)=i*2 : g(i+128)=128-(i*2) : b(i+128)=i*4 : Next
For i=0 To 63 : r(i+192)=128+(i*2) : g(i+192)=i : b(i+192)=255-(i*4) : Next
For i=0 To 255 : mrgb(i)=((r(i)*$10000)+(g(i)*$100)+b(i))And $FFFFFF : Next

Global wave=0

Function Plasma_Calc()
	wave1 = wave1 + 2
 	If wave1 >= 320 Then wave1 = 0 
 	wave2 = wave2 + 2
 	If wave2 >= 320 Then wave2 = 0
 	wave3 = wave3 + 3
 	If wave3 >= 320 Then wave3 = 0
 	SetBuffer TextureBuffer(plasma_tex)
 	LockBuffer
   	For y = 0 To 319
    	d = cosinus(y + wave2) + cosinus(y + wave3)
    	For x = 0 To 319
     		f = (cosinus(x + wave1) + cosinus(x + y) + d) And $FF
     		WritePixelFast x,y,mrgb(f)
   		Next 
  	Next
 	UnlockBuffer
 	SetBuffer BackBuffer()
End Function