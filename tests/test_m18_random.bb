; Milestone 18 — Random Number Functions: Rnd, Rand, SeedRnd, RndSeed

; ----- SeedRnd / RndSeed -----
Print "=== SeedRnd / RndSeed ==="
SeedRnd 42
Print RndSeed()
; Expected: 42

SeedRnd 1234
Print RndSeed()
; Expected: 1234

; ----- Rnd() — float in [0, 1) -----
Print "=== Rnd() ==="
SeedRnd 7
Local a# = Rnd()
; Must be >= 0.0 and < 1.0 — just print it and verify range visually
Print a
; Also check that Rnd() is not always the same without re-seeding
Local b# = Rnd()
Print b
; a and b should differ

; ----- Rnd(max) — float in [0, max) -----
Print "=== Rnd(max) ==="
SeedRnd 99
Local r# = Rnd(100.0)
Print r
; Expected: a float in [0, 100)

; ----- Rnd(min, max) — float in [min, max) -----
Print "=== Rnd(min,max) ==="
SeedRnd 55
Local r2# = Rnd(10.0, 20.0)
Print r2
; Expected: a float in [10, 20)

; ----- Rand(max) — int in [1, max] -----
Print "=== Rand(max) ==="
SeedRnd 42
Local i = Rand(6)
Print i
; Expected: 1..6

; ----- Rand(min, max) — int in [min, max] -----
Print "=== Rand(min,max) ==="
SeedRnd 42
Local j = Rand(3, 9)
Print j
; Expected: 3..9

; ----- Deterministic sequence (same seed = same results) -----
Print "=== Deterministic ==="
SeedRnd 1
Local x1 = Rand(1000)
Local x2 = Rand(1000)
SeedRnd 1
Local y1 = Rand(1000)
Local y2 = Rand(1000)
Print x1
Print y1
; Expected: x1 = y1
Print x2
Print y2
; Expected: x2 = y2

Print "DONE"
