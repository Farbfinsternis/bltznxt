; 3D-06: Camera Entity
; Opens a window with a dark-blue GL-cleared background via per-camera settings.
; Tests: CreateCamera, CameraRange, CameraClsColor, CameraClsMode, CameraViewport,
;        CameraZoom, CameraProjMode

Graphics3D 800,600,32,1

Local cam = CreateCamera()
CameraRange cam, 1, 1000
CameraClsColor cam, 20, 20, 80      ; dark blue background
CameraClsMode cam, 1, 1

; Position camera back so it looks toward the origin
PositionEntity cam, 0, 0, -5

; Zoom default (1.0 = 90 degree HFOV)
CameraZoom cam, 1

; Perspective projection (default)
CameraProjMode cam, 1

UpdateWorld
RenderWorld
Flip
WaitKey
