; Milestone 46: Pixel Buffer Access
;
; All tests are headless-safe:
;   - LockBuffer on screen/image buffers creates a zeroed pixel array
;   - WritePixel/ReadPixel operate on in-memory RGBA data
;   - CopyPixel copies between two independently locked buffers
;   - SaveBuffer writes a PNG via stb_image_write (no renderer needed)
;   - UnlockBuffer is a no-op without a renderer (headless)
;
; NOTE: "Print (expr) And val" is ambiguous because the parser treats the
; leading "(" as a parenthesised-call delimiter.  Use local variables to
; hold intermediate bitfield results instead.

Graphics 800, 600, 32, 0

; ---- BackBuffer pixel write / read ----
LockBuffer BackBuffer()

WritePixelFast 10, 10, Rgb(255, 0, 0), BackBuffer()
Local col% = ReadPixelFast(10, 10, BackBuffer())
; Packed ARGB: (A<<24)|(R<<16)|(G<<8)|B
Local r% = (col Shr 16) And 255
Local g% = (col Shr 8)  And 255
Local b% = col And 255
Print r%     ; 255
Print g%     ; 0
Print b%     ; 0

; WritePixel with bounds check
WritePixel 20, 20, Rgb(0, 255, 0), BackBuffer()
Local col2% = ReadPixel(20, 20, BackBuffer())
Local g2%   = (col2 Shr 8) And 255
Print g2%    ; 255  (green channel)

UnlockBuffer BackBuffer()

; ---- ImageBuffer pixel write / read ----
Local img% = CreateImage(16, 16)
Local imgbuf% = ImageBuffer(img)

Print BufferWidth(imgbuf)     ; 16
Print BufferHeight(imgbuf)    ; 16

LockBuffer imgbuf
WritePixelFast 0, 0, Rgb(0, 0, 255), imgbuf
Local col3% = ReadPixelFast(0, 0, imgbuf)
Local b3%   = col3 And 255
Print b3%    ; 255  (blue channel)
UnlockBuffer imgbuf

; ---- CopyPixel between two locked buffers ----
; Re-lock imgbuf (pixels from previous unlock are persisted)
LockBuffer imgbuf
LockBuffer BackBuffer()

WritePixelFast 5, 5, Rgb(128, 64, 32), imgbuf
CopyPixel 5, 5, imgbuf, 30, 30, BackBuffer()
Local col4% = ReadPixelFast(30, 30, BackBuffer())
Local r4%   = (col4 Shr 16) And 255
Print r4%    ; 128  (red channel)

UnlockBuffer BackBuffer()
UnlockBuffer imgbuf

; ---- SaveBuffer (no crash sufficient; return value is env-dependent) ----
LockBuffer imgbuf
SaveBuffer imgbuf, "__test_m46_buf__.png"
Print "SaveBuffer OK"
UnlockBuffer imgbuf

FreeImage img
EndGraphics
Print "DONE"
