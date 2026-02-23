; Milestone 39 — Buffer & Flip
; Headless: Graphics may or may not open a real window.
; Cls/Flip are safe no-ops when no renderer is available.
; BackBuffer()/FrontBuffer()/SetBuffer() are purely bookkeeping.

Graphics 800, 600, 32, 0

; Buffer handle tokens
Local bb% = BackBuffer()
Local fb% = FrontBuffer()
Print "BackBuffer = " + Str(bb)     ; 1
Print "FrontBuffer = " + Str(fb)    ; 2

SetBuffer BackBuffer()
Print "SetBuffer OK"

; Clear and present — safe no-ops if no window was created
Cls
Print "Cls OK"

Flip
Print "Flip OK"

Flip 0
Print "Flip 0 OK"

; CopyRect stub — must compile and not crash
CopyRect 0, 0, 100, 100, 200, 200
Print "CopyRect OK"

EndGraphics
Print "DONE"
