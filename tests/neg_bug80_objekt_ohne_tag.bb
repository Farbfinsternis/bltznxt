; BUG-80 - ein Objekt passt nicht in eine Variable ohne Tag: die ist ein Int.
; Das Original meldet hier "Illegal type conversion" (gemessen, Blitz3D 11.8).
Type T
	Field q
End Type
c = New T
Print 1
