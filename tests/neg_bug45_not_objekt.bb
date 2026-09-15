; BUG-45: "Not p" ist in der Referenz (p = 0) - Illegal type conversion
Type T
  Field v
End Type
p.T = New T
If Not p Then Print "nie"
