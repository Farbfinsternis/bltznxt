; BUG-182: Restore sucht sein Label immer im Hauptprogramm, auch in einer
; Funktion - ein Label der Funktion selbst findet es nicht (am Original gemessen).
Function f()
.innen
	Restore innen
End Function
Data 1
f()
