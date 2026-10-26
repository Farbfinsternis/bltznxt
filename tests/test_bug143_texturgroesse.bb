; BUG-143 - Texturen haben Seiten aus Zweierpotenzen wie im Original.
;
; Je Seite aufgerundet, ein geladenes Bild mit dem naechsten Pixel gestreckt.
; Gemessen an Blitz3D 11.8 (build/bug143_20261025/size und stretch): genau
; diese Ausgabe. Vorher blieb CreateTexture(30,20) 30x20.
Graphics3D 320,240,0,2
Dim w(5) : Dim h(5)
w(0)=30 : h(0)=20
w(1)=64 : h(1)=16
w(2)=100 : h(2)=60
w(3)=256 : h(3)=256
w(4)=3 : h(4)=129
w(5)=1 : h(5)=1
For k=0 To 5
	t=CreateTexture(w(k),h(k))
	SetBuffer TextureBuffer(t)
	Print w(k)+"x"+h(k)+": "+TextureWidth(t)+"x"+TextureHeight(t)+" buf "+GraphicsWidth()+"x"+GraphicsHeight()
	SetBuffer BackBuffer()
Next
img=CreateImage(100,60)
SaveImage img,"t143.bmp"
t=LoadTexture("t143.bmp")
Print "geladen 100x60: "+TextureWidth(t)+"x"+TextureHeight(t)
DeleteFile "t143.bmp"
img=CreateImage(3,2)
SetBuffer ImageBuffer(img)
LockBuffer
For y=0 To 1
	WritePixelFast 0,y,$FF0000 : WritePixelFast 1,y,$00FF00 : WritePixelFast 2,y,$0000FF
Next
UnlockBuffer
SetBuffer BackBuffer()
SaveImage img,"t143s.bmp"
t=LoadTexture("t143s.bmp")
Print TextureWidth(t)+"x"+TextureHeight(t)
LockBuffer TextureBuffer(t)
For x=0 To 3
	Print x+": "+Hex(ReadPixelFast(x,0,TextureBuffer(t)) And $FFFFFF)
Next
UnlockBuffer TextureBuffer(t)
c=CreateTexture(3,2)
LockBuffer TextureBuffer(c)
WritePixelFast 3,0,$123456,TextureBuffer(c)
Print Hex(ReadPixelFast(3,0,TextureBuffer(c)) And $FFFFFF)
UnlockBuffer TextureBuffer(c)
DeleteFile "t143s.bmp"
