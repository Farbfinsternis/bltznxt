; BUG-107 - "'Data' can only appear in main program" gilt auch in einem
; If-Block. Vorher wurde der Wert still mit aufgenommen.
If 1 Then
  Data 7
EndIf
