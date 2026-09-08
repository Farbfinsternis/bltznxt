; BUG-51 - "Delete Each <Typ>" loescht alle Objekte eines Typs.
;
; Referenz gemessen an Blitz3D 11.8 (G:\dev\Blitz3D): die Form uebersetzt dort
; nach _bbObjDeleteEach. Sie verlangt einen **Typnamen**, keinen Ausdruck -
; "Delete Each p" ueber eine Objektvariable und ein unbekannter Name werden
; beide mit "Specified name is not a NewType name" abgelehnt. Eine leere Liste
; ist zulaessig.
;
; Der Emitter leert die Liste ueber ihren Kopf: bb_T_Delete haengt den Knoten
; aus, der Kopf rueckt nach. Ein eigener Zeiger auf das naechste Element ist
; deshalb nicht noetig.

Type T
  Field v
End Type

Type U
  Field w
End Type

a.T = New T : a\v = 1
b.T = New T : b\v = 2
c.T = New T : c\v = 3
x.U = New U : x\w = 9

Print "vor:"
For p.T = Each T
  Print p\v
Next

Delete Each T

; alle T sind fort
n = 0
For p.T = Each T
  n = n + 1
Next
Print "verbliebene T: " + n

; ein anderer Typ bleibt unberuehrt
For q.U = Each U
  Print "U: " + q\w
Next

; auf der leeren Liste ist die Form folgenlos
Delete Each T
Print "leer nochmal ok"

; danach ist die Liste wieder befuellbar
d.T = New T : d\v = 7
For p.T = Each T
  Print "neu: " + p\v
Next

; auch innerhalb einer Funktion
Leeren()
n = 0
For p.T = Each T
  n = n + 1
Next
Print "nach Funktion: " + n

Function Leeren()
  Delete Each T
End Function
