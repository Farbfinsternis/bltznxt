; BUG-15 — Funktionen werden vorwaerts deklariert und bekommen den Rueckgabetyp
; aus ihrem Type-Hint. Vorher war jede Funktion "auto" und in Quelltextreihenfolge
; emittiert: wechselseitige Aufrufe und Rekursion waren unmoeglich.

; wechselseitig
Function IsEven%(n%)
  If n = 0 Then Return 1
  Return IsOdd(n - 1)
End Function

Function IsOdd%(n%)
  If n = 0 Then Return 0
  Return IsEven(n - 1)
End Function

; rekursiv
Function Fact%(n%)
  If n <= 1 Then Return 1
  Return n * Fact(n - 1)
End Function

; Rueckgabetypen aus dem Hint
Function Half#(x#)
  Return x / 2.0
End Function

Function Greet$(who$)
  Return "hallo " + who
End Function

; ohne Hint = int, wie in Blitz3D
Function NoHint(n%)
  Return n * 2
End Function

; blankes Return und gar kein Return liefern den Standardwert
Function MaybeZero%(flag%)
  If flag = 0 Then Return
  Return 42
End Function

Function NoReturnAtAll%(n%)
  Print n
End Function

Print IsEven(10)
Print IsOdd(10)
Print Fact(6)
Print Half(7)
Print Greet("welt")
Print NoHint(21)
Print MaybeZero(0)
Print MaybeZero(1)
Print NoReturnAtAll(7)
