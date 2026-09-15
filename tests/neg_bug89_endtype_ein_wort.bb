; BUG-89: "EndType" in einem Wort ist in compiler/toker.cpp kein Schluesselwort,
; sondern ein Bezeichner - nur "End Type" schliesst den Typ
Type Punkt
  Field x
EndType
