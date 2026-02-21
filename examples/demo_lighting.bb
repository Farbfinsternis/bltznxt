; ==============================================================================
; BltzNext Lighting & Primitives Demo
; ==============================================================================

Graphics3D 800, 600

; --- Setup Camera ---
cam = CreateCamera()
CameraZoom cam, 1.6
PositionEntity cam, 0, 1, -5 ; Lifted camera slightly

; --- Setup Lighting ---
AmbientLight 32, 32, 32 

; Main light (Yellowish sun)
sun = CreateLight(1) ; Directional
LightColor sun, 255, 240, 200
TurnEntity sun, 45, 45, 0

; Point light (Red glow)
redLight = CreateLight(2) ; Point
LightColor redLight, 255, 0, 0
PositionEntity redLight, -5, 5, -5

; --- Create Objects ---

; 1. Flat Shaded Cube (Red)
cube = CreateCube()
PositionEntity cube, -2, 1.1, 5  ; Lifted to avoid floor intersection
EntityColor cube, 255, 50, 50

; 2. Smooth Shaded Sphere (Blue)
sphere = CreateSphere(16)
PositionEntity sphere, 2, 1.1, 5 ; Lifted
EntityColor sphere, 50, 50, 255

; 3. Floor (Grey Cube scaled down)
floor_obj = CreateCube()
PositionEntity floor_obj, 0, -1.5, 5 ; Lowered
ScaleEntity floor_obj, 10, 0.1, 10
EntityColor floor_obj, 50, 50, 50

; --- Main Loop ---
DebugLog "Starting Main Loop"

While Not KeyDown(1) ; Escape (Hold to exit)

    ; Rotate Objects
    TurnEntity cube, 1, 1, 0
    TurnEntity sphere, 0, 1, 0

    ; Camera Movement (Arrows)
    If KeyDown(203) Then MoveEntity cam, -0.1, 0, 0 ; Left
    If KeyDown(205) Then MoveEntity cam,  0.1, 0, 0 ; Right
    If KeyDown(200) Then MoveEntity cam,  0, 0, 0.1 ; Up (Forward)
    If KeyDown(208) Then MoveEntity cam,  0, 0, -0.1; Down (Back)

    RenderWorld

    ; --- 2D Text Overlay ---
    Color 255, 255, 255
    Text 10, 10, "BltzNext Lighting Demo"
    Text 10, 25, "Arrows: Move Camera"
    Text 10, 40, "ESC: Exit"

    Flip

Wend

DebugLog "Exiting..."
WaitKey ; Wait for key press before closing window
End
