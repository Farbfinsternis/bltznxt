; BUG-39, die Gegenseite - was als Select-Ausdruck erlaubt bleibt.
; ty->structType() ist nur bei einem Type wahr; Zahlen, Strings und das
; Null-Literal sind davon nicht betroffen, und ein Feld eines Objekts ist
; selbst kein Objekt.

Type Punkt
  Field x%
End Type

Global aufrufe% = 0

Function Wert%()
  aufrufe = aufrufe + 1
  Return 4
End Function

Local p.Punkt = New Punkt
p\x = 5

Local a% = 1
Select a
  Case 1
    Print "1: zahl"
End Select

Local s$ = "ja"
Select s
  Case "ja"
    Print "2: string"
End Select

Select Null
  Default
    Print "3: null-literal"
End Select

Select Wert()
  Case 4
    Print "4: aufruf"
End Select

Select p\x
  Case 5
    Print "5: feld eines objekts"
End Select

Print "DONE"
