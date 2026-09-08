; BUG-51 - "Delete Each" verlangt einen Typnamen. Ein unbekannter Name wird
; abgelehnt; das Original meldet "Specified name is not a NewType name"
; (gemessen, Blitz3D 11.8). Dasselbe gilt fuer eine Objektvariable.
Type T
  Field v
End Type
Delete Each Q
