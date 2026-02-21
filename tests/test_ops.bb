
; Test Operators
Print "Testing Operators..."

; Arithmetic
Print "Mod 10, 3 = " + (10 Mod 3) ; Expect 1
Print "Mod -10, 3 = " + (-10 Mod 3) ; Expect -1 (C++ behavior)

; Bitwise
a = 15      ; 1111
b = 3       ; 0011
Print "15 And 3 = " + (a And b)     ; Expect 3 (0011)
Print "15 Or 3 = " + (a Or b)       ; Expect 15 (1111)
Print "15 Xor 3 = " + (a Xor b)     ; Expect 12 (1100)

; Shift
x = 16      ; 10000
Print "16 Shl 1 = " + (x Shl 1)     ; Expect 32
Print "16 Shr 1 = " + (x Shr 1)     ; Expect 8

; Sar (Signed Shift)
neg = -16
Print "-16 Sar 1 = " + (neg Sar 1)  ; Expect -8 (Arithmetic shift)
Print "-16 Shr 1 = " + (neg Shr 1)  ; Expect large positive arithmetic shift usually

; Logic
If 1 And 1 Then Print "Logic And OK"
If 1 Or 0 Then Print "Logic Or OK"

Print "Done."
WaitKey
