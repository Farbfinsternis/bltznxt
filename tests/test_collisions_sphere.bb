Graphics3D 800,600,32,2
SetBuffer BackBuffer()

; Create a camera
cam = CreateCamera()
PositionEntity cam, 0, 5, -15
RotateEntity cam, 20, 0, 0

; Create two spheres
sphere1 = CreateSphere()
PositionEntity sphere1, -5, 0, 0
EntityColor sphere1, 255, 0, 0
EntityType sphere1, 1 ; Type 1
EntityRadius sphere1, 1

sphere2 = CreateSphere()
PositionEntity sphere2, 5, 0, 0
EntityColor sphere2, 0, 255, 0
EntityType sphere2, 2 ; Type 2
EntityRadius sphere2, 1

; Set up collision: Type 1 collides with Type 2
; Method 1 = Sphere-to-Sphere
; Response 1 = Stop
Collisions 1, 2, 1, 1

While Not KeyHit(1)
	; Move sphere1 towards sphere2
	If KeyDown(205) Then MoveEntity sphere1, 0.1, 0, 0 ; Right
	If KeyDown(203) Then MoveEntity sphere1, -0.1, 0, 0 ; Left
	
	UpdateWorld()
	
	RenderWorld()
	
	Color 255, 255, 255
	Text 10, 10, "Use Left/Right to move Red Sphere."
	Text 10, 30, "Red Sphere X: " + EntityX(sphere1)
	Text 10, 50, "Green Sphere X: " + EntityX(sphere2)
	
	Flip
Wend
End
