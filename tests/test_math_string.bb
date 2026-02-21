; Math Tests
Print "Testing Math..."

; Sin/Cos/Tan (Degrees)
If Abs(Sin(0) - 0.0) > 0.001 RuntimeError "Sin(0) failed"
If Abs(Sin(90) - 1.0) > 0.001 RuntimeError "Sin(90) failed"
If Abs(Cos(0) - 1.0) > 0.001 RuntimeError "Cos(0) failed"
If Abs(Cos(180) + 1.0) > 0.001 RuntimeError "Cos(180) failed"
If Abs(Tan(45) - 1.0) > 0.001 RuntimeError "Tan(45) failed"

; Inverse Trig
If Abs(ASin(1.0) - 90.0) > 0.001 RuntimeError "ASin(1) failed"
If Abs(ACos(0.0) - 90.0) > 0.001 RuntimeError "ACos(0) failed"
If Abs(ATan(1.0) - 45.0) > 0.001 RuntimeError "ATan(1) failed"
If Abs(ATan2(1.0, 1.0) - 45.0) > 0.001 RuntimeError "ATan2(1,1) failed"

; Basics
If Abs(Sqr(16.0) - 4.0) > 0.001 RuntimeError "Sqr(16) failed"
If Floor(1.9) <> 1.0 RuntimeError "Floor(1.9) failed"
If Ceil(1.1) <> 2.0 RuntimeError "Ceil(1.1) failed"


; String Tests
Print "Testing Strings..."
s$ = "Hello World"

; Left/Right/Mid
If Left(s$, 5) <> "Hello" RuntimeError "Left failed"
If Right(s$, 5) <> "World" RuntimeError "World failed"
If Mid(s$, 7, 5) <> "World" RuntimeError "Mid failed"

; Upper/Lower
If Upper("abc") <> "ABC" RuntimeError "Upper(abc) failed"
If Lower("XYZ") <> "xyz" RuntimeError "Lower(XYZ) failed"

; Trim
If Trim("  test  ") <> "test" RuntimeError "Trim failed"

; Replace
If Replace("Hello World", "World", "Blitz") <> "Hello Blitz" RuntimeError "Replace failed"

; Instr
If Instr(s$, "World") <> 7 RuntimeError "Instr failed: " + Instr(s$, "World")
If Instr(s$, "o") <> 5 RuntimeError "Instr(o) failed"
If Instr(s$, "o", 6) <> 8 RuntimeError "Instr(o, 6) failed"

; Len/Asc/Chr
If Len(s$) <> 11 RuntimeError "Len failed"
If Asc("A") <> 65 RuntimeError "Asc(A) failed"
If Chr(66) <> "B" RuntimeError "Chr(66) failed"

Print "Math & String Tests Passed"
End
