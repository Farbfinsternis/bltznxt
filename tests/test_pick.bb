
Graphics3D 800, 600, 0, 2
SetBuffer BackBuffer()

cam = CreateCamera()
PositionEntity cam, 0, 0, -5

light = CreateLight()
RotateEntity light, 45, 45, 0

sphere = CreateSphere()
PositionEntity sphere, 0, 0, 0
EntityPickMode sphere, 1 ; Sphere picking
EntityRadius sphere, 1.0

; Text 
font = LoadFont("arial", 16)
SetFont font

While Not KeyHit(1)
    
    If KeyDown(203) Then TurnEntity sphere, 0, 1, 0 ; Left
    If KeyDown(205) Then TurnEntity sphere, 0, -1, 0 ; Right

    RenderWorld

    ; Pick center of screen
    picked = CameraPick(cam, 400, 300)
    
    Color 255, 255, 255
    Text 10, 10, "Picked: " + picked
    If picked = sphere Then
        Text 10, 30, "HIT SPHERE!"
        DebugLog "HIT SPHERE! Dist=" + PickedEntity()
        Text 10, 50, "X: " + PickedX() + " Y: " + PickedY() + " Z: " + PickedZ()
        Text 10, 70, "NX: " + PickedNX() + " NY: " + PickedNY() + " NZ: " + PickedNZ()
        
        Color 255, 0, 0
        ; Draw crosshair
        Rect 395, 299, 10, 2, 1
        Rect 399, 295, 2, 10, 1
    EndIf
    
    Flip
    ; For automated testing, we just run one frame or a few
    count = count + 1
    If count > 10 Then End
Wend
End
