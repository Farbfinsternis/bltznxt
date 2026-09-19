; BUG-173 - Licht wird vor der Textur auf 1 begrenzt, wie in der festen
; Pipeline von Direct3D 7 (Vertexfarbe gesaettigt, dann Texturstufen).
;
; Werte am Original gemessen (2026-09-19). Bei uns wurde die Textur vor dem
; Begrenzen mit dem Licht multipliziert: Textur 100,150,200 unter Licht 1.5
; ergab 150,225,255. In der BirdDemo waren besonnte Flaechen 13% zu hell.

Graphics3D 320,240,0,2
SetBuffer BackBuffer()
cam=CreateCamera()
CameraClsColor cam,0,0,0
tex=CreateTexture(4,4)
SetBuffer TextureBuffer(tex) : ClsColor 100,150,200 : Cls : SetBuffer BackBuffer() : ClsColor 0,0,0
lt=CreateLight(1)
RotateEntity lt,0,0,0
Function M$(t$)
	RenderWorld
	LockBuffer BackBuffer()
	p=ReadPixelFast(160,120) And $FFFFFF
	UnlockBuffer BackBuffer()
	Return t+" "+((p Shr 16) And 255)+","+((p Shr 8) And 255)+","+(p And 255)
End Function
c=CreateCube()
PositionEntity c,0,0,4
AmbientLight 0,0,0
Print M("ohne textur, licht 1")
AmbientLight 128,128,128
Print M("ohne textur, licht 1.5")
EntityTexture c,tex
AmbientLight 0,0,0
Print M("textur, licht 1")
AmbientLight 128,128,128
Print M("textur, licht 1.5")
EntityColor c,128,128,128
Print M("textur, farbe .5, licht 1.5")
EntityColor c,255,128,0
AmbientLight 64,32,16
Print M("textur, farbe 255,128,0, amb 64,32,16")
LightColor lt,100,100,100
AmbientLight 50,50,50
EntityColor c,255,255,255
Print M("textur, licht .59")
