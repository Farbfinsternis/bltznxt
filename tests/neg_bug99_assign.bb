; BUG-99 - "Constants can not be assigned to", auch in einer Funktion ohne
; Local gleichen Namens. Vorher scheiterte erst g++.
Const c = 1
Function F()
  c = 2
End Function
F()
