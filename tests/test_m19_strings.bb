; Milestone 19 — String Extraction & Search

; ----- Left -----
Print "=== Left ==="
Print Left("Hello World", 5)
; Expected: Hello
Print Left("Hi", 10)
; Expected: Hi  (n > len → full string)
Print Left("Hi", 0)
; Expected: (empty)

; ----- Right -----
Print "=== Right ==="
Print Right("Hello World", 5)
; Expected: World
Print Right("Hi", 10)
; Expected: Hi
Print Right("Hi", 0)
; Expected: (empty)

; ----- Mid (2-arg) -----
Print "=== Mid(s,pos) ==="
Print Mid("Hello World", 7)
; Expected: World
Print Mid("Hello", 1)
; Expected: Hello

; ----- Mid (3-arg) -----
Print "=== Mid(s,pos,n) ==="
Print Mid("Hello World", 7, 3)
; Expected: Wor
Print Mid("Hello", 2, 3)
; Expected: ell

; ----- Instr (2-arg) -----
Print "=== Instr(s,sub) ==="
Print Instr("Hello World", "World")
; Expected: 7
Print Instr("Hello World", "xyz")
; Expected: 0
Print Instr("abcabc", "b")
; Expected: 2 (first occurrence)

; ----- Instr (3-arg) -----
Print "=== Instr(s,sub,start) ==="
Print Instr("abcabc", "b", 3)
; Expected: 5 (second 'b', searching from pos 3)

; ----- Replace -----
Print "=== Replace ==="
Print Replace("Hello World", "World", "Blitz")
; Expected: Hello Blitz
Print Replace("aabbcc", "b", "X")
; Expected: aaXXcc

; ----- Len (regression) -----
Print "=== Len ==="
Print Len("Hello")
; Expected: 5
Print Len("")
; Expected: 0

; ----- Str / Int / Float (regression) -----
Print "=== Str/Int/Float ==="
Print Str(42)
; Expected: 42
Print Str(3.14)
; Expected: 3.14
Local n = Int("99")
Print n
; Expected: 99
Local f# = Float("1.5")
Print f
; Expected: 1.5

; ----- Str concatenation in expressions -----
Print "=== Concatenation ==="
Local s$ = "Score: " + Str(100)
Print s
; Expected: Score: 100

Print "DONE"
