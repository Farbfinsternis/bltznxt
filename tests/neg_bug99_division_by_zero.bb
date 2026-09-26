; BUG-99 - Division durch eine konstante Null im Wert eines Const meldet das
; Original schon beim Uebersetzen ("Division by zero").
Const c = 1 Mod 0
Print c
