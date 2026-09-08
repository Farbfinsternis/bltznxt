; BUG-56 - Nach dem Punkt eines Objektparameters muss ein Typname stehen.
; Das Original lehnt "Function F(p.)" ebenfalls ab (gemessen, Blitz3D 11.8).
Function F(p.)
  Return 1
End Function
