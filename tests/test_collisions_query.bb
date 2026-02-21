Graphics3D 800,600,32,2
SetBuffer BackBuffer()

; Create a camera
cam = CreateCamera()
PositionEntity cam, 0, 5, -15

; Create a cube (Obstacle Type 2)
cube = CreateCube()
ScaleEntity cube, 2, 2, 2
PositionEntity cube, 0, 0, 0
EntityColor cube, 100, 100, 255
EntityType cube, 2

; Create a player sphere (Source Type 1)
player = CreateSphere()
ScaleEntity player, 0.5, 0.5, 0.5
PositionEntity player, -5, 0.5, 0
EntityColor player, 255, 0, 0
EntityType player, 1
EntityRadius player, 1

; Set up collision
Collisions 1, 2, 2, 2 ; Sphere-to-Mesh, Slide

While Not KeyHit(1)
	; Move player right
	If KeyDown(205) Then MoveEntity player, 0.1, 0, 0
	If KeyDown(203) Then MoveEntity player, -0.1, 0, 0
	
	UpdateWorld()
	
	RenderWorld()
	
	; Query collisions
	num = CountCollisions(player)
	
	Color 255, 255, 255
	Text 10, 10, "Collisions: " + num
	
	If num > 0
		For i = 1 To num
			Text 10, 30 + i*60, "Col " + i + ": Entity " + CollisionEntity(player, i)
			Text 10, 45 + i*60, "Pos: " + CollisionX(player, i) + ", " + CollisionY(player, i) + ", " + CollisionZ(player, i)
			Text 10, 60 + i*60, "Norm: " + CollisionNX(player, i) + ", " + CollisionNY(player, i) + ", " + CollisionNZ(player, i)
			Text 10, 75 + i*60, "Surf/Tri: " + CollisionSurface(player, i) + " / " + CollisionTriangle(player, i)
		Next
	EndIf
	
	Flip
Wend
End
