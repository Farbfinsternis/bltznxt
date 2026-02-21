Graphics3D 800, 600, 32, 2
SetBuffer BackBuffer()

camera = CreateCamera()
PositionEntity camera, 0, 5, -10

light = CreateLight()
RotateEntity light, 45, 45, 0

; Create Primitives
cube = CreateCube()
PositionEntity cube, -4, 0, 0
EntityType cube, 1

sphere = CreateSphere(12)
PositionEntity sphere, -2, 0, 0
EntityType sphere, 1

cyl = CreateCylinder(12)
PositionEntity cyl, 0, 0, 0
EntityType cyl, 1

cone = CreateCone(12)
PositionEntity cone, 2, 0, 0
EntityType cone, 1

plane = CreatePlane()
PositionEntity plane, 0, -2, 0
EntityType plane, 2

; Test EntityBox
EntityBox cube, -1, -1, -1, 2, 2, 2

; Test Collisions (Stub)
Collisions 1, 2, 2, 2

Print "Primitives created successfully!"
Print "MilliSecs: " + MilliSecs()

Delay 1000
End
