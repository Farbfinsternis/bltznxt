; test_camera.bb - Verifying Fog, Viewport, and Range

Graphics3D 800, 600, 0, 2

cam = CreateCamera()
CameraClsColor cam, 50, 50, 100

; Create some objects to see depth
cube = CreateCube()
PositionEntity cube, 0, 0, 5
EntityColor cube, 255, 0, 0

sphere = CreateSphere()
PositionEntity sphere, 0, 0, 15
EntityColor sphere, 0, 255, 0

pivot = CreatePivot()
PositionEntity pivot, 0, 0, 30
; (Need meshes to see fog well, but pivots/colors work for testing range)

; 1. Test CameraRange
CameraRange cam, 0.1, 10
DebugLog "--- CameraRange Test ---"
DebugLog "Sphere should be culled (distance 15 > far 10)"
RenderWorld
; RenderWorld
; (No WaitKey)

CameraRange cam, 0.1, 100
DebugLog "Sphere should be visible now"
RenderWorld
; RenderWorld
; (No WaitKey)

; 2. Test CameraFog
CameraFogMode cam, 1
CameraFogColor cam, 50, 50, 100
CameraFogRange cam, 5, 20
DebugLog "--- Fog Test ---"
DebugLog "Cube (dist 5) should be clear red"
DebugLog "Sphere (dist 15) should be foggy/blended"
RenderWorld
; RenderWorld
; (No WaitKey)

; 3. Test CameraViewport
CameraFogMode cam, 0
CameraViewport cam, 100, 100, 400, 300
DebugLog "Viewport should be 400x300 at (100,100)"
RenderWorld
Flip
End
