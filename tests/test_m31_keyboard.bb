; Milestone 31 — Keyboard Input
; Headless verification: API compiles and returns correct defaults.
; KeyDown / KeyHit return 0 when no key is pressed (SDL not initialized).

Print "KeyDown(1) = " + Str(KeyDown(1))    ; Escape not held → 0
Print "KeyHit(1) = "  + Str(KeyHit(1))     ; not pressed → 0
FlushKeys
Print "FlushKeys OK"
Print "DONE"
