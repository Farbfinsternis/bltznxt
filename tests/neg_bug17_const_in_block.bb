; BUG-17 - dieselbe Regel gilt fuer Const: "'Const' can only appear in main
; program". Vorher wurde ein blockinternes constexpr emittiert, das ausserhalb
; des Blocks nicht mehr sichtbar war.
Local a% = 1
While a < 2
  Const K% = 5
  a = a + 1
Wend
