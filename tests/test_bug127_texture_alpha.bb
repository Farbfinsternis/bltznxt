; BUG-127 - welches Alpha ein Texturpuffer behaelt.
;
; Am Original gemessen (2026-09-17) fuer die Flags 0 bis 7, mit WritePixelFast
; unter LockBuffer und mit WritePixel: das geschriebene Alpha bleibt nur,
; wenn die Textur einen Alphakanal hat - Flag 4, oder Flag 1 und 2 zusammen
; (blitz3d/texture.cpp setzt fuer Flag 4 Farbe und Alpha). Sonst liest es sich
; als FF. Im Bild macht Alpha 0 die Textur dann durchsichtig.

Graphics3D 320,240,0,2
Dim fl(7)
fl(0)=0 : fl(1)=1 : fl(2)=2 : fl(3)=3 : fl(4)=4 : fl(5)=5 : fl(6)=6 : fl(7)=7
For i = 0 To 7
	t = CreateTexture(8,8,fl(i))
	LockBuffer TextureBuffer(t)
	WritePixelFast 3,3,$40112233,TextureBuffer(t)
	v = ReadPixelFast(3,3,TextureBuffer(t))
	UnlockBuffer TextureBuffer(t)
	WritePixel 4,4,$40112233,TextureBuffer(t)
	LockBuffer TextureBuffer(t)
	w = ReadPixelFast(4,4,TextureBuffer(t))
	UnlockBuffer TextureBuffer(t)
	Print "flags " + fl(i) + " fast " + Hex(v) + " writepixel " + Hex(w)
	FreeTexture t
Next
End
