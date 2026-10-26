; BUG-207 - Shl, Shr und Sar nehmen die Weite modulo 32 wie x86.
;
; Gemessen an Blitz3D 11.8 (build/review20261013/g2): genau diese Ausgabe.
; In C++ ist eine Weite ab 32 undefiniert, g++ faltete sie zu 0 oder -1.
n = 33 : m = -1 : k = 32
Print 1 Shl n
Print 1 Shl m
Print 1 Shl k
Print -1 Shr k
Print -1 Shr n
Print -16 Sar n
Print -16 Sar k
Print 5 Shl 32
Print $80000000 Sar 40
For i = 30 To 34
	Print 3 Shl i
Next
