; BUG-115 - Trim entfernt am Rand jedes Zeichen, das nicht sichtbar ist.
;
; Referenz: bbTrim in bbruntime/bbstring.cpp prueft beide Raender mit
; isgraph(). Im ASCII-Bereich faellt damit alles von 0 bis 32 und 127 weg,
; also auch Nullbyte, vertikaler Tab und Formfeed - nicht nur Leerzeichen,
; Tab, CR und LF. Die Werte 0 bis 127 sind am laufenden Original gemessen
; (2026-09-16).

t$ = ""
For k = 0 To 127
	If Len(Trim(Chr(k) + "x" + Chr(k))) = 1 Then t$ = t$ + " " + k
Next
Print "abgeschnitten:" + t$

Print Len(Trim(Chr(11) + "x" + Chr(12)))
Print "[" + Trim(Chr(0) + Chr(11) + Chr(1)) + "]"
Print Len(Trim(" a" + Chr(11) + "b "))

; Bytes ab 128 liest das Original ausserhalb der isgraph-Tabelle; zwei Laeufe
; lieferten verschiedene Ergebnisse. Festgelegt (Entscheidung 2026-09-16):
; sie gelten als sichtbar und bleiben stehen. Dieser Teil ist deshalb nicht
; gegen das Original geprueft.
n = 0
For k = 128 To 255
	If Len(Trim(Chr(k) + "x" + Chr(k))) = 3 Then n = n + 1
Next
Print "stehen gelassen ab 128: " + n
