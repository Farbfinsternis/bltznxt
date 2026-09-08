; BUG-47 - Zwei verschiedene Type-Tags an derselben Variablen sind ein Fehler.
; Das Original meldet "Variable type mismatch" (gemessen, Blitz3D 11.8).
Type T
  Field v
End Type
Type U
  Field w
End Type
p.T = New T
p.U = New U
