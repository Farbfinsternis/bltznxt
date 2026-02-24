; Milestone 45: Image Manipulation
;
; All tests are headless-safe:
;   - Handle offsets are pure metadata (no renderer needed)
;   - Scale/Rotation are stored but not rendered
;   - Overlap/Collision tests are pure integer arithmetic
;   - Draw/Tile/Ellipse calls are silent no-ops in headless mode
;   - SaveImage returns 0 headlessly (no renderer)

Graphics 800, 600, 32, 0

; ---- Handle system ----
Local img% = CreateImage(64, 64)
HandleImage img, 10, 20
Print ImageXHandle(img)     ; 10
Print ImageYHandle(img)     ; 20

MidHandle img
Print ImageXHandle(img)     ; 32
Print ImageYHandle(img)     ; 32

; ---- AutoMidHandle ----
AutoMidHandle True
Local img2% = CreateImage(32, 16)
Print ImageXHandle(img2)    ; 16
Print ImageYHandle(img2)    ; 8
AutoMidHandle False

; Create a non-auto-midhandled image after turning off
Local img3% = CreateImage(20, 10)
Print ImageXHandle(img3)    ; 0  (AutoMidHandle is off)

; ---- Scale / Rotation (stored silently, no output) ----
ScaleImage img, 2.0, 3.0
RotateImage img, 45.0
; No crash = OK

; ---- MaskImage (no pixel copy on CreateImage → no-op, no crash) ----
MaskImage img, 255, 0, 255

; ---- Draw operations — silent no-ops in headless mode ----
DrawImage img, 100, 100
DrawBlock img, 150, 100
TileImage img, 0, 0
TileBlock img, 0, 0
DrawImageEllipse img, 200, 200, 50, 30
Print "Draw OK"

; ---- SaveImage — no crash is sufficient (return value is env-dependent) ----
SaveImage img, "__test_save__.png"
Print "SaveImage OK"

; ---- Overlap / Collision (bounding-box, pure math) ----
; a: 32x32, handle (0,0). Draw at (0,0) → AABB (0,0)-(32,32)
; b: 32x32, handle (0,0). Draw at (16,16) → AABB (16,16)-(48,48)
Local a% = CreateImage(32, 32)
Local b% = CreateImage(32, 32)

Print ImagesOverlap(a, 0, 0, b, 16, 16)    ; 1 (overlap at 16-32)
Print ImagesOverlap(a, 0, 0, b, 64, 64)    ; 0 (no overlap)

; ImageRectOverlap: a at (10,10) → AABB (10,10)-(42,42)
Print ImageRectOverlap(a, 10, 10, 0, 0, 50, 50)     ; 1
Print ImageRectOverlap(a, 10, 10, 100, 100, 50, 50)  ; 0

; ImagesColl = bounding-box alias
Print ImagesColl(a, 0, 0, b, 16, 16)   ; 1
Print ImagesColl(a, 0, 0, b, 64, 64)   ; 0

; ImageXColl / ImageYColl → centre of overlap region
; a at (0,0)-(32,32), b at (16,16)-(48,48)
; overlap: x=(16,32) centre=24 ; y=(16,32) centre=24
Print ImageXColl(a, 0, 0, b, 16, 16)   ; 24
Print ImageYColl(a, 0, 0, b, 16, 16)   ; 24
; no overlap → 0
Print ImageXColl(a, 0, 0, b, 64, 64)   ; 0

; ---- Cleanup ----
FreeImage img
FreeImage img2
FreeImage img3
FreeImage a
FreeImage b

EndGraphics
Print "DONE"
