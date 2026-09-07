; Milestone 33 — Joystick Input
; Headless verification: API compiles and returns correct defaults.
; No joystick connected (SDL not initialized) → all queries return 0 / 0.0.

Print "JoyType(0) = " + Str(JoyType(0))      ; no device → 0
Print "JoyX(0) = "    + Str(JoyX(0))          ; no device → 0
Print "JoyY(0) = "    + Str(JoyY(0))          ; no device → 0
Print "JoyHat(0) = "  + Str(JoyHat(0))        ; no device → 0
Print "JoyDown(1,0) = " + Str(JoyDown(1, 0))  ; not held  → 0
Print "JoyHit(1,0) = "  + Str(JoyHit(1, 0))   ; not hit   → 0
FlushJoy 0
Print "FlushJoy OK"
Print "DONE"
