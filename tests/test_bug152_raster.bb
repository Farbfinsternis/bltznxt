; BUG-152 - Pixelmitte wie Direct3D 7 (ganze Koordinaten), nicht wie OpenGL
; (.5): jede Kante, die nicht auf einer Pixelgrenze liegt, faellt im Original
; einen halben Pixel weiter rechts und weiter unten.
;
; Gemessen am Original (2026-09-17, build/raster20260917/): dasselbe Quadrat
; gerade, gedreht, um Bruchteile eines Pixels verschoben und schraeg gesehen.

Graphics3D 320,240,0,2
cam = CreateCamera()
CameraClsColor cam,40,40,40
PositionEntity cam,0,0,-5
Function Aus$(x,y)
	LockBuffer BackBuffer()
	l=-1 : r=-1 : o=-1 : u=-1
	For i=0 To 319
		If (ReadPixelFast(i,y) And $FFFFFF)<>$282828
			If l<0 Then l=i
			r=i
		EndIf
	Next
	For i=0 To 239
		If (ReadPixelFast(x,i) And $FFFFFF)<>$282828
			If o<0 Then o=i
			u=i
		EndIf
	Next
	UnlockBuffer BackBuffer()
	Return l+","+r+" / "+o+","+u
End Function
m = CreateMesh()
sf = CreateSurface(m)
AddVertex sf,-1,1,0 : AddVertex sf,1,1,0 : AddVertex sf,1,-1,0 : AddVertex sf,-1,-1,0
AddTriangle sf,0,1,2 : AddTriangle sf,0,2,3
EntityFX m,1 : EntityColor m,255,0,0
RenderWorld
Print "1 mesh gerade: " + Aus(160,120)
RotateEntity m,0,0,30
RenderWorld
Print "2 mesh roll 30: " + Aus(160,120)
RotateEntity m,0,0,0
PositionEntity m,0.3,0.2,0
RenderWorld
Print "3 mesh verschoben 0.3,0.2: " + Aus(160,120)
PositionEntity m,0,0,0
PositionEntity cam,4,0,-4
PointEntity cam,m
RenderWorld
Print "4 kamera seitlich: " + Aus(160,120)
; Bruchteile gezielt: Kante bei x = 160 + 32*k/8
PositionEntity cam,0,0,-5
RotateEntity cam,0,0,0
For k=1 To 7
	PositionEntity m,k/64.0,k/64.0,0
	RenderWorld
	Print "5 versatz " + k + "/8 px: " + Aus(170,110)
Next
End
