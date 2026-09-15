; BUG-90 - der Zaehler einer For-Each-Schleife ist eine gewoehnliche Variable.
;
; Blitz3D kennt keinen Blockbereich: nach "For q.P = Each P" ist q eine
; Variable des Rumpfs vom Typ .P. ForEachNode::translate (compiler/stmtnode.cpp)
; schreibt ueber _bbObjEachFirst/_bbObjEachNext (bbruntime/basic.cpp) in genau
; diese Variable: nach dem letzten Durchlauf haelt sie Null, nach Exit das
; Objekt, bei dem abgebrochen wurde.
;
; Bis 2026-09-15 deklarierte der Emitter den Zaehler innerhalb der
; C++-Schleife. Eine spaetere Zuweisung ohne Tag scheiterte in g++, und ein
; Global oder Parameter als Zaehler wurde nie beschrieben - dieses Programm
; stuerzte mit dem alten Compiler ohne Fall 1 ab.

Type P
  Field x
End Type
For i = 1 To 4
  n.P = New P
  n\x = i
Next

; 1) der gemeldete Fall
For q.P = Each P
Next
q = First P
Print "neu zugewiesen: " + q\x

; 2) nach normalem Ende Null, nach Exit das Objekt
For q = Each P
Next
Print "nach ende null: " + (q = Null)
For q = Each P
  If q\x = 3 Then Exit
Next
Print "nach exit: " + q\x

; 3) Global als Zaehler
Global g.P
For g = Each P
  If g\x = 2 Then Exit
Next
Print "global: " + g\x
Function ZeigeGlobal()
  Print "global in funktion: " + g\x
End Function
ZeigeGlobal()

; 4) Parameter als Zaehler, in einer Funktion
Function Zaehle(p.P)
  c = 0
  For p = Each P
    c = c + 1
  Next
  Return c
End Function
Print "zaehle: " + Zaehle(Null)

; 5) Delete im Rumpf
For q = Each P
  If q\x Mod 2 = 0 Then Delete q
Next
s$ = ""
For q = Each P
  s = s + q\x
Next
Print "nach delete: " + s

; 6) verschachtelt, zwei Zaehler
k = 0
For a.P = Each P
  For b.P = Each P
    k = k + 1
  Next
Next
Print "paare: " + k
