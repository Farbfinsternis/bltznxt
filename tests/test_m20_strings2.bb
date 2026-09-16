; Milestone 20 — String Transformation & Encoding

; ----- Upper / Lower -----
Print "=== Upper/Lower ==="
Print Upper("Hello World")
; Expected: HELLO WORLD
Print Lower("Hello World")
; Expected: hello world

; ----- Trim -----
Print "=== Trim ==="
Print Trim("  hello  ")
; Expected: hello
Print Trim("no spaces")
; Expected: no spaces
Print Trim("   ")
; Expected: (empty)

; ----- LSet / RSet -----
Print "=== LSet/RSet ==="
Print LSet("Hi", 8)
; Expected: "Hi      " (padded to 8)
Print RSet("Hi", 8)
; Expected: "      Hi" (right-aligned in 8)
Print LSet("Truncated", 5)
; Expected: Trunc
Print RSet("Truncated", 5)
; Expected: Trunc

; ----- Chr / Asc -----
Print "=== Chr/Asc ==="
Print Chr(65)
; Expected: A
Print Chr(97)
; Expected: a
Print Asc("A")
; Expected: 65
Print Asc("Hello")
; Expected: 72 (ASCII of 'H')

; ----- Hex -----
Print "=== Hex ==="
Print Hex(255)
; Expected: 000000FF (always 8 digits, BUG-111)
Print Hex(16)
; Expected: 00000010
Print Hex(0)
; Expected: 00000000
Print Hex(65535)
; Expected: 0000FFFF

; ----- Bin -----
Print "=== Bin ==="
Print Bin(0)
; Expected: 32 zeros (always 32 digits, BUG-111)
Print Bin(1)
; Expected: 31 zeros, then 1
Print Bin(5)
; Expected: 29 zeros, then 101
Print Bin(255)
; Expected: 24 zeros, then 11111111

; ----- String (repeat) -----
Print "=== String ==="
Print String("ab", 3)
; Expected: ababab
Print String("-", 5)
; Expected: -----
Print String("x", 0)
; Expected: (empty)

; ----- Combinations -----
Print "=== Combos ==="
Local h$ = "0x" + Hex(255)
Print h
; Expected: 0x000000FF
Local padded$ = RSet(Str(42), 6)
Print padded
; Expected: "    42"

Print "DONE"
