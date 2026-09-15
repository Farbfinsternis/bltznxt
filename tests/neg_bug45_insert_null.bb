; BUG-45: Null ist ein eigener Typ - Insert verlangt zwei gleiche
Type T
  Field v
End Type
p.T = New T
Insert Null Before p
