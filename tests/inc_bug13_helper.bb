; Hilfsdatei fuer test_bug13_include.bb — kein test_*-Praefix, damit der
; Runner sie nicht als eigenen Test einsammelt.
Function Doubled%(n%)
  Return n * 2
End Function
Global INCLUDE_MARKER% = 7
