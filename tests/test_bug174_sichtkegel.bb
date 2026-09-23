; BUG-174 - Was ausserhalb des Sichtkegels liegt, wird weder gezeichnet noch
; von TrisRendered gezaehlt.
;
; Das Original prueft vor dem Zeichnen jedes Modells dessen Huelle gegen den
; Kegel der Kamera: ein Mesh ueber seine Huellbox (MeshModel::render), ein
; Sprite ueber seine vier Ecken (Sprite::render). Verworfen wird nur, wenn
; alle Punkte ausserhalb derselben Ebene liegen - eine Box schraeg vor einer
; Ecke des Kegels zaehlt deshalb noch mit. Der Kegel haengt nicht von der
; Projektion ab: auch die orthografische Kamera verwirft mit dem
; perspektivischen Kegel (Camera::getFrustum).
;
; Gemessen am Original (2026-09-23, build/cull20260923). Vorher zaehlten wir
; alles, was nicht versteckt war.

Graphics3D 320,240,0,2
SetBuffer BackBuffer()
cam = CreateCamera()

Function Z(name$)
	RenderWorld
	Print name$ + ": " + TrisRendered()
End Function

c = CreateCube()
Z("wuerfel vorn")
PositionEntity c,0,0,-5 : Z("hinter der kamera")

; Ueber die linke Ebene (90 Grad: x = -z)
For i = 0 To 16
	x# = -10 - i * 0.25
	PositionEntity c,x,0,10
	Z("links x=" + x)
Next
; Schraeg ueber die Ecke oben links
For i = 0 To 16
	x# = -10 - i * 0.25
	PositionEntity c,x,-x*0.75,10
	Z("ecke x=" + x)
Next
; Nahe Ebene
For i = 0 To 8
	z# = -0.8 + i * 0.2
	PositionEntity c,0,0,z
	Z("nah z=" + z)
Next
; Ferne Ebene
CameraRange cam,1,50
For i = 0 To 8
	z# = 50.2 + i * 0.2
	PositionEntity c,0,0,z
	Z("fern z=" + z)
Next
CameraRange cam,1,1000

; Gedreht, skaliert, Kamera gedreht, Zoom
PositionEntity c,-12,0,10 : Z("aussen")
TurnEntity c,0,45,0 : Z("gedreht 45")
ScaleEntity c,3,1,1 : Z("skaliert")
RotateEntity c,0,0,0 : ScaleEntity c,1,1,1
PositionEntity c,10,0,0 : Z("rechts")
TurnEntity cam,0,-90,0 : Z("rechts, kamera gedreht")
RotateEntity cam,0,0,0
PositionEntity c,-4,0,10
CameraZoom cam,2 : Z("zoom 2")
CameraZoom cam,4 : Z("zoom 4")
CameraZoom cam,1

; Schmaler Viewport: der Kegel wird flacher
CameraViewport cam,0,0,320,60
For i = 0 To 8
	y# = 2.6 + i * 0.1
	PositionEntity c,0,y,10
	Z("viewport y=" + y)
Next
CameraViewport cam,0,0,320,240

; Kind eines Pivots
p = CreatePivot()
EntityParent c,p,False
PositionEntity c,0,0,10
PositionEntity p,-30,0,0 : Z("kind aussen")
PositionEntity p,0,0,0 : Z("kind innen")
FreeEntity p

; Orthografisch mit dem Kegel der perspektivischen Kamera
c = CreateCube()
ScaleEntity c,0.05,0.05,0.05
CameraRange cam,0.1,100
CameraProjMode cam,2
For i = 0 To 10
	x# = i * 0.1
	PositionEntity c,x,0,0.5
	Z("ortho x=" + x)
Next
CameraProjMode cam,1
CameraRange cam,1,1000
FreeEntity c

; Geometrie nach dem ersten Zeichnen geaendert
m = CreateMesh()
s = CreateSurface(m)
AddVertex s,0,0,10 : AddVertex s,1,0,10 : AddVertex s,0,1,10
AddTriangle s,0,1,2
Z("dreieck innen")
VertexCoords s,0,-50,0,10 : VertexCoords s,1,-49,0,10 : VertexCoords s,2,-50,1,10
Z("dreieck verschoben")
ClearSurface s
AddVertex s,0,0,10 : AddVertex s,1,0,10 : AddVertex s,0,1,10
AddTriangle s,0,1,2
Z("nach ClearSurface")
m2 = CreateMesh()
Z("leeres mesh")
FreeEntity m : FreeEntity m2

; Sprites ueber ihre vier Ecken
sp = CreateSprite()
For i = 0 To 8
	x# = -10.6 + -i * 0.1
	PositionEntity sp,x,0,10
	Z("sprite x=" + x)
Next
PositionEntity sp,0,0,-5 : Z("sprite hinten")
SpriteViewMode sp,2
PositionEntity sp,-11.5,0,10
RotateEntity sp,0,-90,0 : Z("sprite fest, gedreht")
RotateEntity sp,0,0,0 : Z("sprite fest, gerade")

End
