Graphics3D 800,600,32,2
SetBuffer BackBuffer()

; Create a camera
cam = CreateCamera()
PositionEntity cam, 0, 5, -15
RotateEntity cam, 20, 0, 0

; Create a cube mesh
cube = CreateCube()
ScaleEntity cube, 2, 2, 2
PositionEntity cube, 0, 0, 0
EntityColor cube, 100, 100, 255
EntityType cube, 2 ; Type 2 (Obstacle)

; Create a player sphere
player = CreateSphere()
ScaleEntity player, 0.5, 0.5, 0.5
PositionEntity player, -5, 0, 0
EntityColor player, 255, 0, 0
EntityType player, 1 ; Type 1 (Source)
EntityRadius player, 1

; Set up collision: Sphere-to-Mesh
; Method 2 = Sphere-to-Mesh
; Response 2 = Slide
Collisions 1, 2, 2, 2

While Not KeyHit(1)
	; Control player with arrows
	If KeyDown(200) Then MoveEntity player, 0, 0, 0.1  ; Up
	If KeyDown(208) Then MoveEntity player, 0, 0, -0.1 ; Down
	If KeyDown(203) Then MoveEntity player, -0.1, 0, 0 ; Left
	If KeyDown(205) Then MoveEntity player, 0.1, 0, 0  ; Right
	
	UpdateWorld()
	
	RenderWorld()
	
	Color 255, 255, 255
	Text 10, 10, "Use Arrows to move Red Sphere into the blue Cube."
	Text 10, 30, "Sphere-to-Mesh (Method 2), Slide (Response 2)"
	Text 10, 50, "Player X: " + EntityX(player)
	Text 10, 70, "Player Z: " + EntityZ(player)
	
	Flip
Wend
End
