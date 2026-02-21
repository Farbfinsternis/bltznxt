Graphics3D 640,480,0,2
cam = CreateCamera()
PositionEntity cam, 0, 0, -5
cube = CreateCube()
PositionEntity cube, 0, 0, 5

While Not KeyDown(1)
    If KeyDown(203) Then MoveEntity cam, -0.1, 0, 0
    If KeyDown(205) Then MoveEntity cam, 0.1, 0, 0
    If KeyDown(200) Then MoveEntity cam, 0, 0, 0.1
    If KeyDown(208) Then MoveEntity cam, 0, 0, -0.1
    
    RenderWorld
    Text 10, 10, "Cam X: " + EntityX(cam)
    Text 10, 25, "Cam Y: " + EntityY(cam)
    Text 10, 40, "Cam Z: " + EntityZ(cam)
    Flip
Wend
End
