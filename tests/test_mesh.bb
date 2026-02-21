Graphics3D 800,600,32,2
SetBuffer BackBuffer()

camera = CreateCamera()
PositionEntity camera, 0, 0, -5

light = CreateLight()
RotateEntity light, 45, 45, 0

; Test 1: Dynamic Mesh Creation
mesh = CreateMesh()
surf = CreateSurface(mesh)

v0 = AddVertex(surf, -1, -1, 0, 0, 1)
v1 = AddVertex(surf,  1, -1, 0, 1, 1)
v2 = AddVertex(surf,  0,  1, 0, 0.5, 0)

AddTriangle(surf, v0, v1, v2)
UpdateNormals mesh
EntityColor mesh, 255, 0, 0
PositionEntity mesh, -2, 0, 0

; Test 2: Primitives
cube = CreateCube()
EntityColor cube, 0, 255, 0
PositionEntity cube, 0, 0, 0

sphere = CreateSphere(16)
EntityColor sphere, 0, 0, 255
PositionEntity sphere, 2, 0, 0

; Test 3: New Primitives
cyl = CreateCylinder(16)
EntityColor cyl, 255, 255, 0
PositionEntity cyl, -2, -2, 0
ScaleEntity cyl, 0.5, 0.5, 0.5

cone = CreateCone(16)
EntityColor cone, 0, 255, 255
PositionEntity cone, 2, -2, 0
ScaleEntity cone, 0.5, 0.5, 0.5

While Not KeyHit(1)
    TurnEntity mesh, 0, 1, 0
    TurnEntity cube, 1, 1, 1
    TurnEntity sphere, 0, 1, 0
    TurnEntity cyl, 1, 0, 0
    TurnEntity cone, 0, 0, 1
    
    RenderWorld
    Text 400, 20, "Phase 6: Meshes & Geometry Test", 1
    Flip
Wend
End
