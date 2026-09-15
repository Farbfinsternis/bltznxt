; BUG-40: parseVar() liest "\" nur hinter einer Variablen, nicht hinter einer Klammer
Type Node
  Field val
End Type
Local a.Node = New Node
Print (First Node)\val
