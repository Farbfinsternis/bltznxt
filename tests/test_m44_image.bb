; Milestone 44: Image Loading & Drawing
;
; CreateImage works headlessly (stores dimensions, tex=nullptr).
; LoadImage with a non-existent file returns 0 (graceful failure).
; All Draw* calls are safe no-ops in headless mode.

Graphics 800, 600, 32, 0

; CreateImage: always returns a valid handle
Local img% = CreateImage(64, 64)
Print ImageWidth(img)
Print ImageHeight(img)

; Draw operations: silent no-ops in headless mode (no renderer)
DrawImage img, 10, 10
Print "DrawImage OK"

DrawImageRect img, 10, 10, 0, 0, 32, 32
Print "DrawImageRect OK"

DrawBlock img, 20, 20
Print "DrawBlock OK"

DrawBlockRect img, 20, 20, 0, 0, 16, 16
Print "DrawBlockRect OK"

; FreeImage: no crash
FreeImage img

; After free, width/height return 0
Print ImageWidth(img)

; LoadImage: non-existent file → returns 0
Local img2% = LoadImage("__nonexistent_image__.png")
Print img2

EndGraphics
Print "DONE"
