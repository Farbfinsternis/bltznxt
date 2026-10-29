; BUG-224 - Ein widersprechendes Objekt-Tag in einem Ausdruck ist ein Fehler.
; Das Original meldet "Variable type mismatch" (gemessen).
Type T
  Field v
End Type
Type U
  Field w
End Type
Local p.T = New T
If p.U = Null Then Print "null"
