; Milestone 41 — Line & Shape Primitives
; Headless: no window is created; all drawing calls are safe no-ops.
; Line/Rect/Oval/Poly are verified to compile and call without crashing.

Graphics 800, 600, 32, 0

Color 255, 0, 0

; --- Line ---
Line 0, 0, 100, 100
Line 200, 0, 0, 200
Print "Line OK"

; --- Rect ---
Rect 50, 50, 40, 40, 1    ; filled
Rect 10, 10, 80, 80, 0    ; outline
Print "Rect OK"

; --- Oval ---
Oval 100, 100, 60, 40, 1  ; filled ellipse
Oval 200, 200, 80, 80, 0  ; outline circle
Print "Oval OK"

; --- Poly (filled triangle) ---
Poly 0, 0, 100, 0, 50, 100
Print "Poly OK"

; --- Default solid param (both default to solid=1) ---
Color 0, 255, 0
Rect 20, 20, 30, 30
Oval 120, 120, 30, 30
Print "Defaults OK"

EndGraphics
Print "DONE"
