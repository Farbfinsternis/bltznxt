Graphics3D 640,480,0,2
cam = CreateCamera()
PositionEntity cam, 0, 0, -5
cube = CreateCube()
PositionEntity cube, 0, 0, 5

cubeX = 0.0

While Not KeyHit(1)
    TurnEntity cube, 1, 1, 0
    If KeyDown(203) Then cubeX = cubeX - 0.1
    If KeyDown(205) Then cubeX = cubeX + 0.1
    PositionEntity cube, cubeX, 0, 5
    RenderWorld
    Flip
Wend
End
