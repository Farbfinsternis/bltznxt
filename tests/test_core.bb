; =============================================================================
; tests/test_core.bb — Comprehensive Core BlitzBasic Test
; =============================================================================

Print "--- Starting Core Tests ---"

; 1. Strings
Print "Testing Strings..."
s$ = "  Hello BltzNxt!  "
Print "Trim: '" + Trim(s$) + "'"
Print "Left: " + Left(Trim(s$), 5)
Print "Right: " + Right(Trim(s$), 7)
Print "Mid: " + Mid(Trim(s$), 7, 7)
Print "Instr: " + Instr(s$, "Bltz")
Print "Upper: " + Upper("hello")
Print "Lower: " + Lower("WORLD")
Print "Hex: " + Hex(255)
Print "Bin: " + Bin(5)

; 2. Math & Random
Print ""
Print "Testing Math (Degrees)..."
Print "Sin(90): " + Sin(90) ; Should be 1.0
Print "Cos(0): " + Cos(0)   ; Should be 1.0
Print "Tan(45): " + Tan(45) ; Should be ~1.0
Print "Sqr(16): " + Sqr(16)
Print "Abs(-42): " + Abs(-42)

Print "Testing Random..."
SeedRnd 12345
r1 = Rand(1, 100)
r2 = Rand(1, 100)
Print "Rand(1,100): " + r1 + ", " + r2

; 3. Banks
Print ""
Print "Testing Banks..."
bank = CreateBank(16)
PokeByte bank, 0, 255
PokeInt bank, 4, 12345678
PokeFloat bank, 8, 3.14159
Print "PeekByte(0): " + PeekByte(bank, 0)
Print "PeekInt(4): " + PeekInt(bank, 4)
Print "PeekFloat(8): " + PeekFloat(bank, 8)
FreeBank bank

; 4. Files
Print ""
Print "Testing Files..."
f = WriteFile("test_data.bin")
WriteLine f, "Hello File System!"
WriteInt f, 987654
WriteFloat f, 1.234
CloseFile f

f = ReadFile("test_data.bin")
Print "Line: " + ReadLine(f)
Print "Int: " + ReadInt(f)
Print "Float: " + ReadFloat(f)
CloseFile f

; 5. Arrays (Multi-type)
Print ""
Print "Testing Arrays..."
Dim a(10)
Dim b#(10)
Dim c$(10)

a(5) = 100
b#(5) = 1.5
c$(5) = "Multi-type Array Success!"

Print "a(5): " + a(5)
Print "b(5): " + b#(5)
Print "c(5): " + c$(5)

Print ""
Print "--- Core Tests Complete ---"
Delay 2000
End
