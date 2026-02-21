; 3D Rotating Cube Test
Graphics3D 800, 600
Color 0, 0, 255
    
cube = CreateCube()
PositionEntity cube, 0, 0, 0
    
cam = CreateCamera()
PositionEntity cam, 0, 0, -5

For i = 1 To 120
    TurnEntity cube, 1, 1, 1
    RenderWorld
    Flip
Next
End
