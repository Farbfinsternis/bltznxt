; BUG-94 - Include ist eine Anweisung wie jede andere.
;
; Im Original (parser.cpp) darf Include zwischen Doppelpunkten stehen, und
; dieselbe Datei in anderer Schreibung kommt nur einmal herein. Gemessen an
; Blitz3D 11.8 (build/bug94_20261025/inc): genau diese Ausgabe. Bei uns ging
; der Rest der Zeile nach einem Include verloren ("b", "c" fehlten), ein
; Include hinter einem Doppelpunkt wurde abgelehnt, und "BUG94\H2.BB" lief
; ein zweites Mal.
Print "a" : Include "bug94\h1.bb" : Print "b"
Include "bug94\h2.bb" : Print "c"
Include "BUG94\H2.BB"
Print "x:y" : Include "bug94\h3.bb" ; Kommentar: Include "gibtsnicht.bb"
Print "d"

Function Aus(s$)
	Print s
End Function
