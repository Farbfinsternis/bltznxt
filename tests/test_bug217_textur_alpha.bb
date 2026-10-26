; BUG-217: Texturalpha blendet nur bei genau einer Texturlage (Bumpy)
Graphics3D 200,150,0,2
cam=CreateCamera()
CameraClsColor cam,0,0,0
Restore faelle
Repeat
	Read fc,fs,bl,ea#,n
	If fc=0 Then Exit
	c=CreateTexture(4,4,fc)
	s=CreateTexture(4,4,fs)
	SetBuffer TextureBuffer(c) : ClsColor $7D,$51,$52 : Cls
	SetBuffer TextureBuffer(s) : ClsColor $7F,$7F,$7F : Cls
	SetBuffer BackBuffer()
	LockBuffer TextureBuffer(c)
	For y=0 To 3 : For x=0 To 3 : WritePixelFast x,y,$607D5152,TextureBuffer(c) : Next : Next
	UnlockBuffer TextureBuffer(c)
	LockBuffer TextureBuffer(s)
	For y=0 To 3 : For x=0 To 3 : WritePixelFast x,y,$7F7F7F7F,TextureBuffer(s) : Next : Next
	UnlockBuffer TextureBuffer(s)
	TextureBlend s,bl
	cube=CreateCube()
	PositionEntity cube,0,0,3
	EntityFX cube,1
	EntityAlpha cube,ea
	EntityTexture cube,c,0,0
	If n>=2 Then EntityTexture cube,s,0,1
	If n>=3 Then EntityTexture cube,c,0,2
	RenderWorld
	LockBuffer BackBuffer()
	v=ReadPixelFast(100,75) And $FFFFFF
	UnlockBuffer BackBuffer()
	; Rundung der Grafikkarte: auf 8er-Stufen
	r=((v Shr 16) And 255)/8 : g=((v Shr 8) And 255)/8 : b=(v And 255)/8
	Print fc+" "+fs+" b"+bl+" a"+ea+" n"+n+" -> "+r+","+g+","+b
	FreeEntity cube : FreeTexture c : FreeTexture s
Forever
.faelle
Data 3,1,2,1.0,1
Data 3,1,2,1.0,2
Data 3,1,2,0.5,2
Data 1,3,1,1.0,2
Data 3,3,1,1.0,2
Data 3,3,3,1.0,2
Data 3,1,2,1.0,3
Data 3,3,2,0.5,2
Data 0,0,0,0,0
