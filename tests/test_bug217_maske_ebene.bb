; BUG-217 - nur eine maskierte erste Textur schneidet aus (Brush Tiles).
;
; Eine maskierte Lightmap auf einer spaeteren Ebene liess bei uns jedes
; schwarze Texel das ganze Fragment verwerfen, samples/birdie/Brush Tiles
; zeigte einen schwarzen Boden. Im Original schaltet nur eine maskierte
; Textur auf Ebene 0 den Alphatest ein. Der Hintergrund ist blau, ein
; verworfenes Fragment zeigt Blau statt Schwarz. Erwartung: Blitz3D 11.8.
Graphics3D 200,150,0,2
cam=CreateCamera()
CameraClsColor cam,0,0,255
img=CreateImage(4,4)
SetBuffer ImageBuffer(img) : ClsColor 0,0,0 : Cls
SetBuffer BackBuffer()
f$=SystemProperty("tempdir")+"bltznxt_bug217.bmp"
SaveImage img,f$
lm=LoadTexture(f$,4)
DeleteFile f$
base=CreateTexture(4,4)
SetBuffer TextureBuffer(base) : ClsColor 200,100,50 : Cls
SetBuffer BackBuffer()
For stufe=0 To 3
	cube=CreateCube()
	PositionEntity cube,0,0,3
	EntityFX cube,1
	Select stufe
	Case 0 : EntityTexture cube,base,0,0
	Case 1 : EntityTexture cube,base,0,0 : EntityTexture cube,lm,0,1
	Case 2 : EntityTexture cube,lm,0,0
	Case 3 : EntityTexture cube,lm,0,0 : EntityTexture cube,base,0,1
	End Select
	RenderWorld
	LockBuffer BackBuffer()
	v=ReadPixelFast(100,75) And $FFFFFF
	UnlockBuffer BackBuffer()
	Print stufe+": "+((v Shr 16) And 255)/8+","+((v Shr 8) And 255)/8+","+(v And 255)/8
	FreeEntity cube
Next
