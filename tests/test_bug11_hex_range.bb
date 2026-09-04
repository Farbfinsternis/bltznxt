; BUG-11 — Hex-/Binaerliterale duerfen den Compiler nicht abstuerzen lassen.
; Blitz3D-Integer sind 32 Bit und wrappen: $FFFFFFFF ist -1.
Print $FF
Print $FFFFFFFF
Print $7FFFFFFF
Print $80000000
Print $0
Print %1010
Print %11111111111111111111111111111111
Local a% = $FF
Print a - $FFFFFFFF
Local s$ = "type hint bleibt"
Print s
