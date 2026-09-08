; BUG-53 - Zahlen und Strings wandeln frei, Objekte nie. Das Original meldet
; hier "Illegal type conversion" (gemessen, Blitz3D 11.8); dieselbe Ablehnung
; gilt fuer Objekt an Integer, String an Objekt und zwei verschiedene Types.
Type T
  Field v
End Type
Local s$ = New T
