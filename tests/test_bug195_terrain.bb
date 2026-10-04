; BUG-195 (Terrain) - CreateTerrain, LoadTerrain, ModifyTerrain, TerrainHeight,
; TerrainSize, TerrainDetail, TerrainShading, TerrainX/Y/Z; dazu Kollision und
; Picking gegen Terrain, TrisRendered (ROAM) und Stats3D(1/2).
; Alle Werte am Original gemessen (2026-10-04).

Graphics3D 320, 240, 32, 2
SetBuffer BackBuffer()

Function F$(x#)
	Return Int(Floor(x * 10000 + 0.5))
End Function

; --- A: Groesse, Klasse, Kopie ---
t = CreateTerrain(32)
Print "A " + TerrainSize(t) + " " + EntityClass(t) + " " + EntityClass(CopyEntity(t))

; --- B: Speichern als Byte ---
ModifyTerrain t, 1, 1, 0.5
ModifyTerrain t, 2, 1, 1.0
ModifyTerrain t, 3, 1, 1.5
ModifyTerrain t, 4, 1, -0.25
ModifyTerrain t, 5, 1, 0.999
ModifyTerrain t, 6, 1, 0.0019
ModifyTerrain t, 7, 1, 2.0
s$ = "B"
For x = 1 To 7 : s$ = s$ + " " + F(TerrainHeight(t, x, 1)) : Next
Print s$

; --- C: Rand ---
ModifyTerrain t, 32, 2, 0.6
ModifyTerrain t, 0, 3, 0.7
ModifyTerrain t, 33, 4, 0.8
ModifyTerrain t, -1, 5, 0.9
s$ = "C " + F(TerrainHeight(t, 0, 2)) + " " + F(TerrainHeight(t, 32, 2))
s$ = s$ + " " + F(TerrainHeight(t, 32, 3)) + " " + F(TerrainHeight(t, 33, 3)) + " " + F(TerrainHeight(t, -1, 3))
s$ = s$ + " " + F(TerrainHeight(t, 1, 4)) + " " + F(TerrainHeight(t, 31, 5))
Print s$

; --- D: TerrainX/Y/Z ---
u = CreateTerrain(16)
For z = 0 To 16 : For x = 0 To 16
	ModifyTerrain u, x, z, (Sin(x * 20) * Cos(z * 25) + 1) / 2
Next : Next
s$ = "D0"
For k = 0 To 5
	s$ = s$ + " " + F(TerrainY(u, k * 1.37, 3, k * 2.11))
Next
Print s$
ScaleEntity u, 2, 50, 3
PositionEntity u, -10, -5, 3
TurnEntity u, 0, 30, 0
s$ = "D1"
For k = 0 To 5
	s$ = s$ + " " + F(TerrainX(u, k * 3.1 - 4, 7, k * 2.7)) + "," + F(TerrainY(u, k * 3.1 - 4, 7, k * 2.7)) + "," + F(TerrainZ(u, k * 3.1 - 4, 7, k * 2.7))
Next
Print s$
s$ = "D2 " + F(TerrainY(u, -100, 0, -100)) + " " + F(TerrainY(u, 1000, 0, 1000))
Print s$

; --- E: LoadTerrain, je Kanal ---
img = CreateImage(16, 16)
SetBuffer ImageBuffer(img)
For y = 0 To 15 : For x = 0 To 15
	v = x * 16 + y
	If y < 5
		WritePixel x, y, (v Shl 16) Or $FF000000
	ElseIf y < 10
		WritePixel x, y, (v Shl 8) Or $FF000000
	Else
		WritePixel x, y, v Or $FF000000
	EndIf
Next : Next
SetBuffer BackBuffer()
SaveImage img, "hm.bmp"
w = LoadTerrain("hm.bmp")
For zz = 0 To 15 Step 5
	s$ = "E" + zz
	For x = 0 To 15
		s$ = s$ + " " + Int(Floor(TerrainHeight(w, x, 15 - zz) * 255 + 0.5))
	Next
	Print s$
Next
Print "E " + TerrainSize(w)
HideEntity w

; --- F: Kollision ---
HideEntity t
EntityType u, 2
ball = CreateSphere()
EntityType ball, 1
EntityRadius ball, 0.5
Collisions 1, 2, 2, 2
PositionEntity ball, 0, 60, 10
For k = 1 To 40
	TranslateEntity ball, 0.3, -2, 0.1
	UpdateWorld
Next
Print "F " + F(EntityX(ball)) + " " + F(EntityY(ball)) + " " + F(EntityZ(ball)) + " " + CountCollisions(ball) + " " + (EntityCollided(ball, 2) = u)
If CountCollisions(ball) Then Print "F n " + F(CollisionNX(ball, 1)) + " " + F(CollisionNY(ball, 1)) + " " + F(CollisionNZ(ball, 1))
; Abwurf ueber der Mitte des Terrains, mit Gleiten
TFormPoint 8, 0, 8, u, 0
PositionEntity ball, TFormedX(), 60, TFormedZ()
ResetEntity ball
For k = 1 To 40
	TranslateEntity ball, 0.15, -2, 0.05
	UpdateWorld
Next
Print "F2 " + F(EntityX(ball)) + " " + F(EntityY(ball)) + " " + F(EntityZ(ball)) + " " + CountCollisions(ball) + " " + (EntityCollided(ball, 2) = u) + " " + F(TerrainY(u, EntityX(ball), 0, EntityZ(ball)))
If CountCollisions(ball) Then Print "F2 n " + F(CollisionNX(ball, 1)) + " " + F(CollisionNY(ball, 1)) + " " + F(CollisionNZ(ball, 1))
HideEntity ball

; --- G: Picking ---
EntityPickMode u, 2
For k = 0 To 4
	; Punkte abseits aller Raster- und Diagonalkanten: auf einer Kante entscheidet
	; das letzte Bit (x87 im Original), welches Dreieck trifft
	TFormPoint 2.37 + k * 2.91, 0, 3.71 + k * 2.53, u, 0
	p = LinePick(TFormedX(), 80, TFormedZ(), 0, -200, 0)
	Print "G2 " + (p = u) + " " + F(PickedX()) + " " + F(PickedY()) + " " + F(PickedZ()) + " " + F(PickedNY()) + " " + F(TerrainY(u, PickedX(), 0, PickedZ()))
Next
For k = 0 To 4
	p = LinePick(k * 4 - 8, 80, k * 3 + 2, 0, -200, 0)
	Print "G " + (p = u) + " " + F(PickedX()) + " " + F(PickedY()) + " " + F(PickedZ()) + " " + F(PickedNY())
Next

; --- H: Zeichnen ---
cam = CreateCamera()
PositionEntity cam, 0, 40, -30
PointEntity cam, u
For d = 0 To 3
	If d = 0 Then TerrainDetail u, 2000
	If d = 1 Then TerrainDetail u, 500
	If d = 2 Then TerrainDetail u, 100, 1
	If d = 3 Then TerrainDetail u, 4000, 1 : TerrainShading u, 1
	RenderWorld
	Print "H" + d + " " + TrisRendered() + " " + Stats3D(1) + " " + Stats3D(2)
Next
MoveEntity cam, 0, 0, 25
RenderWorld
Print "H4 " + TrisRendered()
TurnEntity cam, 0, 180, 0
RenderWorld
Print "H5 " + TrisRendered()

; Aendern in Echtzeit und mit spaeterer Neuberechnung
TurnEntity cam, 0, 180, 0
ModifyTerrain u, 8, 8, 1.0, True
RenderWorld
Print "H6 " + TrisRendered()
ModifyTerrain u, 4, 12, 0.0
ModifyTerrain u, 12, 4, 0.9
RenderWorld
Print "H7 " + TrisRendered()
Print "H8 " + F(TerrainY(u, 0, 0, 10)) + " " + F(TerrainHeight(u, 8, 8))

End
