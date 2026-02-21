
; Test Images & Textures
Graphics3D 800, 600, 0, 2

; Load test image (Phase 2.1)
img = LoadImage("test_sprite.tga")
If Not img Then Print "Failed to load test_sprite.tga" : WaitKey : End

; Load test texture (Phase 2.2)
tex = LoadTexture("test_sprite.tga")
If Not tex Then Print "Failed to load texture" : WaitKey : End

cube = CreateCube()
EntityTexture cube, tex
PositionEntity cube, 0, 0, 5

cam = CreateCamera()
light = CreateLight()

Print "Testing 2D and 3D Textures..."
While Not KeyHit(1) ; ESC to exit
    Cls
    
    TurnEntity cube, 0.5, 1, 0.2
    
    RenderWorld
    
    ; 2D Overlay Drawing
    DrawImage img, 10, 10
    DrawImage img, MouseX(), MouseY()
    
    Text 10, 550, "Image Handle: " + img + ", Texture Handle: " + tex
    Text 10, 570, "Move mouse for 2D. Cube is 3D Textured. ESC to exit."
    
    Flip
Wend

FreeImage img
FreeTexture tex
End
