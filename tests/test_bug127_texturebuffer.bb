; BUG-127 - Zeichnen in eine Textur ueber TextureBuffer.
;
; Alle Werte am Original gemessen (2026-09-17): eine frische Textur liest sich
; als $FF000000; Rect, Cls, DrawImage und Viewport wirken wie in einem Image;
; die Aenderung ist beim naechsten RenderWorld auf dem Wuerfel sichtbar, auch
; nach dem ersten; jeder Frame hat seinen eigenen Puffer; eine geladene Textur
; laesst sich bemalen; ReadPixel liefert Alpha immer FF, ReadPixelFast unter
; LockBuffer das geschriebene Alpha.
;
; Nicht hier: dass Alpha 0 im Bild durchsichtig wird (BUG-142, Blending), die
; Rundung der Groesse auf Zweierpotenzen (BUG-143), und Rect in eine
; Alpha-Textur - das bringt das Original zum Absturz.

Graphics3D 320,240,0,2
cam = CreateCamera() : PositionEntity cam,0,0,-2.5 : CameraClsColor cam,0,0,0
cube = CreateCube() : EntityFX cube,1

Function Px$()
	RenderWorld
	Return Hex(ReadPixel(160,120) And $FFFFFF)
End Function

; 1) frische Textur, Groesse
t1 = CreateTexture(32,32)
Print "1 frisch " + Hex(ReadPixel(3,3,TextureBuffer(t1))) + " w " + TextureWidth(t1) + " h " + TextureHeight(t1)
t3 = CreateTexture(32,32,2)
Print "1c alpha frisch " + Hex(ReadPixel(3,3,TextureBuffer(t3)))

; 2) zeichnen und lesen
SetBuffer TextureBuffer(t1)
Color 255,0,0 : Rect 0,0,32,32,1
Color 0,255,0 : Rect 8,8,16,16,1
SetBuffer BackBuffer()
Print "2 rect " + Hex(ReadPixel(2,2,TextureBuffer(t1))) + " " + Hex(ReadPixel(16,16,TextureBuffer(t1)))

; 3) auf dem Wuerfel sichtbar
EntityTexture cube,t1
Print "3 wuerfel mitte " + Px() + " rand " + Hex(ReadPixel(110,120) And $FFFFFF)

; 4) Aenderung nach dem ersten RenderWorld
SetBuffer TextureBuffer(t1)
Color 0,0,255 : Rect 8,8,16,16,1
SetBuffer BackBuffer()
Print "4 nach aenderung " + Px()

; 5) WritePixel mit Alpha, Alpha-Textur und normale
WritePixel 1,1,$80FF0000,TextureBuffer(t3)
WritePixel 1,1,$80FF0000,TextureBuffer(t1)
Print "5 alpha-textur " + Hex(ReadPixel(1,1,TextureBuffer(t3))) + " farbtextur " + Hex(ReadPixel(1,1,TextureBuffer(t1)))

; 5b) gesperrt bleibt das geschriebene Alpha erhalten
LockBuffer TextureBuffer(t3)
WritePixelFast 3,3,$40112233,TextureBuffer(t3)
Print "5b readpixelfast " + Hex(ReadPixelFast(3,3,TextureBuffer(t3)))
UnlockBuffer TextureBuffer(t3)
Print "5c readpixel danach " + Hex(ReadPixel(3,3,TextureBuffer(t3)))

; 6) Cls mit ClsColor, Viewport
SetBuffer TextureBuffer(t1)
ClsColor 255,255,0 : Cls
Viewport 0,0,4,4
Print "6 cls " + Hex(ReadPixel(20,20)) + " ausserhalb viewport " + Hex(ReadPixel(20,20)) + " innen " + Hex(ReadPixel(1,1))
SetBuffer BackBuffer()
ClsColor 0,0,0
Print "6b wuerfel gelb " + Px()

; 7) Frames
ta = CreateTexture(16,16,1,3)
For f = 0 To 2
	SetBuffer TextureBuffer(ta,f)
	Color (f=0)*255,(f=1)*255,(f=2)*255 : Rect 0,0,16,16,1
Next
SetBuffer BackBuffer()
Print "7 frames " + Hex(ReadPixel(1,1,TextureBuffer(ta,0))) + " " + Hex(ReadPixel(1,1,TextureBuffer(ta,1))) + " " + Hex(ReadPixel(1,1,TextureBuffer(ta,2)))
EntityTexture cube,ta,2
Print "7b wuerfel frame 2 " + Px()

; 8) geladene Textur bemalen
tl = LoadTexture("tests/assets/test_grid.png")
SetBuffer TextureBuffer(tl)
Color 255,0,255 : Rect 0,0,4,4,1
SetBuffer BackBuffer()
Print "8 geladen bemalt " + Hex(ReadPixel(1,1,TextureBuffer(tl))) + " w " + TextureWidth(tl)

; 9) Image in Textur zeichnen, Textur-Handle-Werte
img = CreateImage(8,8)
SetBuffer ImageBuffer(img) : Color 255,128,0 : Rect 0,0,8,8,1
SetBuffer TextureBuffer(t1) : DrawImage img,0,0 : SetBuffer BackBuffer()
Print "9 image in textur " + Hex(ReadPixel(2,2,TextureBuffer(t1)))
Print "9b puffer ungleich 0 " + (TextureBuffer(t1) <> 0) + " verschieden " + (TextureBuffer(t1) <> TextureBuffer(t3))
End
