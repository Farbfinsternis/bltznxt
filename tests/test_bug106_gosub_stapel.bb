; BUG-106 - Gosub/Return mit Ruecksprungstapel wie call/ret im Original:
; Verschachtelung ueber drei Ebenen, Rekursion 5000 tief, und ein Return ohne
; offenes Gosub im Hauptprogramm beendet das Programm normal ("nach return"
; darf nicht erscheinen). Vorher: Endlosschleife. Am Original gemessen
; (build/gosub20260918). Blox-n-balls hing daran beim Laden.

Global tiefe = 0
Print "start"
Gosub outer
Print "nach outer"
Gosub rek
Print "nach rek " + tiefe
Gosub a
Print "nach a"
Gosub outer
Print "nochmal"
Goto ende

.outer
Print "outer an"
Gosub inner
Print "outer zurueck"
Gosub inner
Print "outer ende"
Return

.inner
Print "inner"
Return

.rek
tiefe = tiefe + 1
If tiefe < 5000 Then Gosub rek
Return

.a
Print "a"
Gosub b
Print "a ende"
Return
.b
Print "b"
Gosub c
Print "b ende"
Return
.c
Print "c"
Return

.ende
Print "fertig"
Return
Print "nach return"
