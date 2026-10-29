; BUG-226 - Instr und Mid mit Position 0 beenden das Programm wie im Original.
;
; bbInstr und bbMid (bbruntime/bbstring.cpp) pruefen die Position mit CHKOFF
; und werfen "parameter must be greater than 0" - auch ohne Debug. Vorher
; nahmen wir 0 still als 1. Die Grenzwerte davor sind erlaubt und muessen
; weiter gehen; die Werte folgen dem Quelltext des Originals.

s$ = "abc"
Print "[" + Mid(s, 1) + "] [" + Mid(s, 3, 1) + "] [" + Mid(s, 4, 1) + "] [" + Mid(s, 9) + "] [" + Mid(s, 2, -5) + "]"
Print Instr(s, "b", 1) + " " + Instr(s, "b", 3) + " " + Instr(s, "b", 99) + " " + Instr(s, "", 4)
Print "[" + Left(s, 0) + "] [" + Right(s, 0) + "] [" + LSet(s, 0) + "] [" + RSet(s, 0) + "]"
Print "vor dem Fehler"
p = 0
Print Instr(s, "b", p)
Print "nicht erreicht"
