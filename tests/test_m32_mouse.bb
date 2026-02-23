; Milestone 32 — Mouse Input
; Headless verification: API compiles and returns correct defaults.
; MouseX/Y/Z return 0 when SDL is not initialized (no window open).

Print "MouseX() = " + Str(MouseX())       ; no window → 0
Print "MouseY() = " + Str(MouseY())       ; no window → 0
Print "MouseZ() = " + Str(MouseZ())       ; no scroll  → 0
Print "MouseDown(1) = " + Str(MouseDown(1))  ; not held → 0
Print "MouseHit(1) = "  + Str(MouseHit(1))   ; not hit  → 0
FlushMouse
Print "FlushMouse OK"
Print "DONE"
