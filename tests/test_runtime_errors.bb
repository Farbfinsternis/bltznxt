; R2 Verifikation: bb_Int / bb_Float mit ungueltigem Input
; Kein Crash erlaubt — Fallback auf 0 + stderr-Warnung
;
; bb_Int("abc") -> 0, bb_Float("xyz") -> 0.0

Local i% = Int("abc")
Print i                 ; -> 0

Local f# = Float("xyz")
Print f                 ; -> 0
