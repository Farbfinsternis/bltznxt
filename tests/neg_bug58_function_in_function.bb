; BUG-58: fehlt End Function, steht die naechste Function mitten im Rumpf -
; "'Function' can only appear in main program"
Function A()
  Print "a"

Function B()
  Print "b"
End Function
