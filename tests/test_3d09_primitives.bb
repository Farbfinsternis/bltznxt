; 3D-09: Primitive Meshes
; Renders a rotating cube — first visible 3D output.
; Tests: CreateCube, CreateSphere, CreateCylinder, CreateCone,
;        MeshWidth, MeshHeight, MeshDepth, TurnEntity, RenderWorld

Graphics3D 800,600,32,1

Local cam = CreateCamera()
CameraRange cam, 0.1, 1000
CameraClsColor cam, 20, 30, 60
CameraClsMode cam, 1, 1
PositionEntity cam, 0, 0, 6
;RotateEntity cam, 0, 180, 0

Local cube = CreateCube()
PositionEntity cube, -1.5, 0, 0

Local sphere = CreateSphere(12)
PositionEntity sphere, 1.5, 0, 0

; Verify AABB queries
Local w# = MeshWidth(cube)
Local h# = MeshHeight(cube)
Local d# = MeshDepth(cube)

While Not KeyDown(1)
  TurnEntity cube, 1, 1, 0
  TurnEntity sphere, 0, 1, 0.5
  UpdateWorld
  RenderWorld
  Flip
Wend
