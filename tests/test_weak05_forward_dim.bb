; Test WEAK-05: Dim forward reference
; A Function uses an array that is Dim'd AFTER the function declaration.
; Without the pre-scan fix, the parser treats arr(i) as a function call.
; Without the file-scope hoisting fix, the function can't see the vector.

Function SumThree()
  Local s% = arr(0) + arr(1) + arr(2)
  Print s
End Function

; Dim comes AFTER the function that uses it
Dim arr%(4)
arr(0) = 10
arr(1) = 20
arr(2) = 30
SumThree()

; Multi-array Dim, one declared after use
Function PrintPair()
  Print a(0)
  Print b(0)
End Function

Dim a%(2), b%(2)
a(0) = 11
b(0) = 22
PrintPair()
