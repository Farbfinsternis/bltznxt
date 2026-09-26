; BUG-49 - eine Vorgabe mit konstanter Division durch Null meldet das Original
; schon beim Uebersetzen ("Division by zero"). Vorher stuerzte das Programm
; beim Aufruf ab.
Function F(n = 1 / 0)
  Return n
End Function
Print F()
