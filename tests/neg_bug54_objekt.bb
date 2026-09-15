; BUG-54: And wandelt nach int - ein Objekt kann das nicht (Illegal type conversion)
Type T
  Field v
End Type
p.T = New T
Print p And 1
