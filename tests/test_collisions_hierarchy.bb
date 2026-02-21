Graphics3D 800,600,32,2
SetBuffer BackBuffer()

; Create a camera
cam = CreateCamera()
PositionEntity cam, 0, 5, -15

; Create a parent pivot (Type 1)
parent = CreatePivot()
PositionEntity parent, 0, 0, 0
EntityType parent, 1

; Create a child mesh (Type 2)
child = CreateCube(parent)
ScaleEntity child, 2, 2, 2
PositionEntity child, 0, 0, 0
EntityColor child, 100, 255, 100
EntityType child, 2

; Set up collision between Type 1 and Type 2
Collisions 1, 2, 2, 1 ; Stop response

While Not KeyHit(1)
	; Move parent pivot
	If KeyDown(205) Then MoveEntity parent, 0.1, 0, 0
	If KeyDown(203) Then MoveEntity parent, -0.1, 0, 0
	
	UpdateWorld()
	
	RenderWorld()
	
	; Query collisions
	num = CountCollisions(parent)
	
	Color 255, 255, 255
	Text 10, 10, "Parent Collisions: " + num + " (Should be 0)"
	Text 10, 30, "The parent pivot should be able to move THROUGH its child cube"
	Text 10, 50, "without colliding, because of hierarchy exclusion."
	
	Flip
Wend
End
