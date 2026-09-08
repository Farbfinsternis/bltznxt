; BUG-48 - Nach einem einzeiligen If schliesst die Referenz mit einer
; Zeilenendepruefung ab. Ein "EndIf" auf derselben Zeile ist deshalb ein
; Fehler: das Original lehnt sowohl "If a=1 EndIf" als auch
; "If a=1 Print "x" EndIf" ab (gemessen, Blitz3D 11.8).
a = 1
If a = 1 Print "x" EndIf
