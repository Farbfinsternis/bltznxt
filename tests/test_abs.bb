; Test Abs function
i% = -5
f# = -5.5
r1% = Abs(i)
r2# = Abs(f)

Print "Abs(-5) = " + r1
Print "Abs(-5.5) = " + r2

If r1 = 5 Then Print "Integer Abs Correct" Else Print "Integer Abs Failed"
If r2 = 5.5 Then Print "Float Abs Correct" Else Print "Float Abs Failed"

; Check types (runtime check via manual inspection or relying on strict assignment)
; If Abs(i) returned float, r1% assignment might implicitly cast, but we want to ensure source type is correct if possible?
; Transpiler test:
; If Abs(i) is float, generated C++ might look like: r1 = (bb_int)bbAbs((bb_float)i);
; We want: r1 = bbAbs(i); where bbAbs is overloaded.
