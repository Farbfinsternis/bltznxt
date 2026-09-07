; BUG-12 — Blitz3D-Bezeichner sind case-insensitiv: Count, count und COUNT
; sind dieselbe Variable. Das gilt fuer Variablen, Konstanten, Arrays,
; Schleifenvariablen, Typen, Felder, Funktionen, Labels und Kommandos.

Type Enemy
  Field Health%
End Type

Global Total% = 0

Function AddTwo%(Value%)
  Return value + 2
End Function

Function Bump%(N%)
  Total = Total + n
  Return total
End Function

; Variablen
Local Count% = 1
count = count + 1
COUNT = COUNT + 1
Print Count

; Kommandos in beliebiger Schreibweise
print "klein"
PRINT "GROSS"
PrInT "gemischt"

; Benutzerfunktionen
Print addtwo(40)
Print ADDTWO(0)

; Globals aus Funktionen heraus
Print Bump(5)
Print bump(5)
Print TOTAL

; Typen, Felder, New, First, For Each, Delete
Local a.Enemy = New Enemy : a\Health = 1
Local b.Enemy = New enemy : b\HEALTH = 2
For P.ENEMY = Each ENEMY
  Print p\health
Next
Delete A
Print (First ENEMY)\Health

; Arrays und Schleifenvariablen
Dim Board%(3)
board(1) = 5
Print BOARD(1)
For Index% = 1 To 3
  Print index
Next

; Konstanten und Strings
Const MAX% = 7
Print max
Local Name$ = "abc"
Print Upper(name)

; Labels, Goto, Gosub, Restore
Local s$
Restore MyData
Read s$ : Print s

Goto SkipMe
Print "nicht erreicht"
.SKIPME
Print "sprung ok"
Gosub HelpEr
Print "zurueck"
Goto TheEnd

.helper
Print "im gosub"
Return

.TheEnd
Print "fertig"

.MYDATA
Data "aus data"
