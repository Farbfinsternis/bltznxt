Graphics3D 800,600,32,2
SetBuffer BackBuffer()

; Create a camera
cam = CreateCamera()
PositionEntity cam, 0, 10, -20
RotateEntity cam, 30, 0, 0

; Create a "wall" made of spheres
For i = -5 To 5
    wall = CreateSphere()
    PositionEntity wall, i*2, 0, 5
    EntityColor wall, 100, 100, 100
    EntityType wall, 2 ; Type 2 (Obstacle)
    EntityRadius wall, 1
Next

; Create the player sphere
player = CreateSphere()
PositionEntity player, 0, 0, -5
EntityColor player, 255, 0, 0
EntityType player, 1 ; Type 1 (Source)
EntityRadius player, 1

; Set up collision: Type 1 slides against Type 2
; Method 1 = Sphere-to-Sphere
; Response 2 = Slide
Collisions 1, 2, 1, 2

While Not KeyHit(1)
	; Control player with arrows
	If KeyDown(200) Then MoveEntity player, 0, 0, 0.1  ; Up
	If KeyDown(208) Then MoveEntity player, 0, 0, -0.1 ; Down
	If KeyDown(203) Then MoveEntity player, -0.1, 0, 0 ; Left
	If KeyDown(205) Then MoveEntity player, 0.1, 0, 0  ; Right
	
	UpdateWorld()
	
	RenderWorld()
	
	Color 255, 255, 255
	Text 10, 10, "Use Arrows to move Red Sphere into the wall."
	Text 10, 30, "Response: SLIDE. It should glide along the wall."
	Text 10, 50, "Player Z: " + EntityZ(player)
	
	Flip
Wend
End
