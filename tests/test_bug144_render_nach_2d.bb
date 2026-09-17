; BUG-144 - das erste RenderWorld nach 2D-Befehlen, die den Kontext des
; SDL-Renderers aktivieren.
;
; CreateImage, LoadImage und DrawImage legen SDL-Texturen an und machen dabei
; den GL-Kontext des 2D-Renderers aktuell. RenderWorld uebersetzte seine Shader
; beim ersten Aufruf, bevor es den eigenen Kontext setzte - die Programme lagen
; dann im falschen Kontext, und ReadPixel, LockBuffer und CopyRect lasen Weiss.
; Die Werte sind am Original gemessen (2026-09-17).

Graphics3D 320,240,0,2
a = CreateImage(4,4)
b = LoadImage("tests/assets/test_grid.png")
DrawImage b,0,0
cam = CreateCamera() : PositionEntity cam,0,0,-3 : CameraClsColor cam,40,0,0
cube = CreateCube() : EntityFX cube,1 : EntityColor cube,0,200,0
RenderWorld
Print "readpixel mitte " + Hex(ReadPixel(160,120)) + " ecke " + Hex(ReadPixel(318,238))
LockBuffer BackBuffer()
Print "lockbuffer " + Hex(ReadPixelFast(160,120))
UnlockBuffer BackBuffer()
tex = CreateTexture(16,16)
CopyRect 152,112,16,16,0,0,BackBuffer(),TextureBuffer(tex)
Print "copyrect in textur " + Hex(ReadPixel(8,8,TextureBuffer(tex)))
End
