; BUG-88: ein ganzes festes Array ist kein Objekt, auch wenn seine Elemente es sind
Type Node
  Field val
End Type
Local nodes.Node[2]
If Before nodes = Null Then Print "nie"
