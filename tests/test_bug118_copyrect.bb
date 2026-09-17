; BUG-118 - CopyRect kopiert zwischen beliebigen Puffern.
;
; Wie bbCopyRect im Original ein blit ohne Maske (gxcanvas.cpp): der Handle
; der Quelle verschiebt, Origin und Viewport des Ziels gelten, beide Puffer
; sind ohne Angabe der aktuelle. Alle Werte am Original gemessen (2026-09-17):
; Image, Textur und Backbuffer als Quelle und Ziel, Clipping am Bildrand.
;
; Der Fall Backbuffer -> Textur steht vorn, vor dem ersten CreateImage: danach
; liest ReadPixel auf dem Bildschirm Weiss (BUG-144).

Graphics3D 320,240,0,2

; 4) Backbuffer -> Textur nach RenderWorld
SetBuffer BackBuffer()
cam = CreateCamera() : PositionEntity cam,0,0,-3 : CameraClsColor cam,40,0,0
cube = CreateCube() : EntityFX cube,1 : EntityColor cube,0,200,0
RenderWorld
tex = CreateTexture(64,64)
CopyRect 128,88,64,64,0,0,BackBuffer(),TextureBuffer(tex)
Print "4 backbuffer->textur mitte " + Hex(ReadPixel(32,32,TextureBuffer(tex))) + " ecke " + Hex(ReadPixel(0,0,TextureBuffer(tex)))

; 5) Textur -> Backbuffer
SetBuffer TextureBuffer(tex) : Color 0,0,255 : Rect 0,0,64,64,1 : SetBuffer BackBuffer()
Cls
CopyRect 0,0,16,16,300,5,TextureBuffer(tex),BackBuffer()
Print "5 textur->backbuffer " + Hex(ReadPixel(305,10)) + " geklippt rechts " + Hex(ReadPixel(319,10)) + " daneben " + Hex(ReadPixel(299,10))

; 1) Image -> Image (Nachweis aus der Buglist)
a = CreateImage(4,4) : b = CreateImage(4,4)
SetBuffer ImageBuffer(a) : Color 255,0,0 : Rect 0,0,4,4,1
SetBuffer BackBuffer()
CopyRect 0,0,4,4,0,0,ImageBuffer(a),ImageBuffer(b)
Print "1 image->image " + Hex(ReadPixel(1,1,ImageBuffer(b)))

; 2) Handle der Quelle, Origin des Ziels, Clipping
src = CreateImage(8,8)
SetBuffer ImageBuffer(src) : Color 0,255,0 : Rect 0,0,8,8,1 : Color 0,0,0 : Rect 2,2,2,2,1
HandleImage src,2,2
dst = CreateImage(16,16)
SetBuffer ImageBuffer(dst) : ClsColor 0,0,255 : Cls
Origin 1,1
CopyRect 0,0,8,8,10,10,ImageBuffer(src),ImageBuffer(dst)
Origin 0,0
s$ = ""
For y = 0 To 15 : For x = 0 To 15
	c = ReadPixel(x,y) And $FFFFFF
	If c = $FF Then s = s + "b" Else If c = $FF00 Then s = s + "g" Else If c = 0 Then s = s + "." Else s = s + "?"
Next : s = s + "|" : Next
Print "2 " + s
ClsColor 0,0,0

; 3) Vorgaben: beide Puffer = aktueller Puffer
SetBuffer ImageBuffer(dst) : Cls : Color 255,255,0 : Rect 0,0,2,2,1
CopyRect 0,0,2,2,5,5
Print "3 innerhalb " + Hex(ReadPixel(5,5))

; 6) Image -> Backbuffer mit Origin und Viewport auf dem Backbuffer
SetBuffer BackBuffer()
Cls
Origin 20,20
Viewport 0,0,25,25
CopyRect 0,0,8,8,0,0,ImageBuffer(src),BackBuffer()
Origin 0,0
Viewport 0,0,320,240
Print "6 origin " + Hex(ReadPixel(20,20)) + " viewport " + Hex(ReadPixel(24,24)) + " ausserhalb " + Hex(ReadPixel(26,26))

; 7) Backbuffer -> Backbuffer
Cls
Color 255,0,255 : Rect 0,0,4,4,1
CopyRect 0,0,4,4,50,50
Print "7 backbuffer->backbuffer " + Hex(ReadPixel(51,51))

; 8) Textur -> Image, Image -> Textur
img = CreateImage(8,8)
CopyRect 0,0,8,8,0,0,TextureBuffer(tex),ImageBuffer(img)
Print "8 textur->image " + Hex(ReadPixel(1,1,ImageBuffer(img)))
CopyRect 0,0,8,8,10,10,ImageBuffer(src),TextureBuffer(tex)
Print "8b image->textur " + Hex(ReadPixel(10,10,TextureBuffer(tex))) + " " + Hex(ReadPixel(12,12,TextureBuffer(tex)))
End
