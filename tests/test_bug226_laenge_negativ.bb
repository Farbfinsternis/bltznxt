; BUG-226 - Left, Right, LSet und RSet mit negativer Laenge beenden das
; Programm wie im Original.
;
; bbLeft, bbRight, bbLSet und bbRSet (bbruntime/bbstring.cpp) pruefen die
; Laenge mit CHKPOS und werfen "parameter must be positive" - auch ohne
; Debug. Vorher lieferten wir still einen leeren String.

Print "[" + Left("abc", 0) + "] [" + Left("abc", 9) + "]"
n = -1
Print "[" + Left("abc", n) + "]"
Print "nicht erreicht"
