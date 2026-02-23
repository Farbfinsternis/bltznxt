; Milestone 17 — Math Completeness: Trig Inverse, Sgn, Log10, Pi, Int(float)

; ----- Pi constant -----
Print "=== Pi ==="
Print Pi
; Expected: 3.14159...

; ----- Existing trig (regression) -----
Print "=== Sin/Cos/Tan ==="
Print Sin(0)
Print Cos(0)
Print Tan(45)
; Expected: 0 / 1 / 1

; ----- Inverse trig (new) -----
Print "=== ASin/ACos/ATan/ATan2 ==="
Print ASin(1.0)
; Expected: 90.0
Print ACos(1.0)
; Expected: 0.0
Print ATan(1.0)
; Expected: 45.0
Print ATan2(1.0, 1.0)
; Expected: 45.0

; ----- Sgn -----
Print "=== Sgn ==="
Print Sgn(-42.5)
; Expected: -1
Print Sgn(0)
; Expected: 0
Print Sgn(7.3)
; Expected: 1

; ----- Log10 -----
Print "=== Log10 ==="
Print Log10(100.0)
; Expected: 2.0
Print Log10(1000.0)
; Expected: 3.0

; ----- Int(float) — truncate toward zero -----
Print "=== Int ==="
Print Int(3.9)
; Expected: 3
Print Int(-3.9)
; Expected: -3

; ----- Floor/Ceil regression -----
Print "=== Floor/Ceil ==="
Print Floor(3.7)
; Expected: 3
Print Ceil(3.2)
; Expected: 4

; ----- Sqr/Abs regression -----
Print "=== Sqr/Abs ==="
Print Sqr(16.0)
; Expected: 4
Print Abs(-5.5)
; Expected: 5.5

; ----- Pi in expressions -----
Print "=== Pi in expr ==="
Local r# = Pi * 2.0
Print r
; Expected: 6.28318...

Print "DONE"
