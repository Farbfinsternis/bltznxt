; BUG-57 - Zwei Leerzeichen fassen nicht zusammen: "Else  If" ist ein
; verschachteltes If und braucht ein eigenes EndIf. Mit nur einem EndIf lehnt
; auch das Original ab (gemessen, Blitz3D 11.8).
a = 2
If a = 1
  Print "eins"
Else  If a = 2
  Print "zwei"
EndIf
