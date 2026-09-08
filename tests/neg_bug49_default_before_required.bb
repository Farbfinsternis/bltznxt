; BUG-49 - Eine Vorgabe vor einem Parameter ohne Vorgabe ist erlaubt, aber nie
; weglassbar: Pflicht ist alles bis zum letzten Parameter ohne Vorgabe. Das
; Original meldet hier "Not enough parameters" (gemessen, Blitz3D 11.8).
Function F(a = 1, b)
  Return a + b
End Function
Print F(1)
