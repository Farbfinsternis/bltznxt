; Milestone 40 — Color & Pixel Primitives
; Headless: no window is created; all drawing calls are safe no-ops.
; Color/ClsColor state is pure integer storage — always testable.

Graphics 800, 600, 32, 0

; --- Color() and ColorRed/Green/Blue() ---
Color 200, 100, 50
Print "ColorRed = " + Str(ColorRed())     ; 200
Print "ColorGreen = " + Str(ColorGreen()) ; 100
Print "ColorBlue = " + Str(ColorBlue())   ; 50

; --- ClsColor() ---
ClsColor 10, 20, 30
Print "ClsColor OK"

; Cls uses the new clear color — safe no-op headless
Cls
Print "Cls OK"

; --- Color clamping (values > 255 and < 0 clamp to 255/0) ---
Color 300, -5, 128
Print "Clamped Red = " + Str(ColorRed())   ; 255
Print "Clamped Green = " + Str(ColorGreen()) ; 0
Print "Clamped Blue = " + Str(ColorBlue())   ; 128

; --- Plot (safe no-op headless) ---
Color 255, 0, 0
Plot 100, 100
Print "Plot OK"

; --- GetColor (safe no-op headless; does not change state when no renderer) ---
Color 77, 88, 99
GetColor 100, 100
Print "GetColor OK"

EndGraphics
Print "DONE"
