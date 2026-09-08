; BUG-49 - Ein Vorgabewert muss ein konstanter Ausdruck sein. Eine Variable
; lehnt das Original mit "Expression must be constant" ab (gemessen).
Global g = 3
Function F(x = g)
  Return x
End Function
Print F()
