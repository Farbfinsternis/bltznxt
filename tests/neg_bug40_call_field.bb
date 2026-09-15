; BUG-40: auch ein Funktionsaufruf ist keine Variable, "\" dahinter bleibt liegen
Type Node
  Field val
End Type
Function Neu.Node()
  Return New Node
End Function
Print Neu()\val
