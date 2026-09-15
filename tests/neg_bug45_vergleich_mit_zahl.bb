; BUG-45: ein Objekt vergleicht nur mit Objekten und Null
Type T
  Field v
End Type
p.T = New T
If p = 0 Then Print "nie"
