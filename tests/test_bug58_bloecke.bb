; BUG-58 - die Gegenprobe: gueltige Blockformen und das Programmende in Bloecken.
;
; Nachgeschlagen in compiler/parser.cpp (2026-09-15): parseStmtSeq() endet an
; jedem Token, das keine Anweisung beginnt; nur der passende Abschluss schliesst
; den Block. Ein blosses "End" ist immer das Programmende - bis 2026-09-15
; beendete es bei uns den Rumpf einer Funktion bzw. ein Select, und der Rest
; lief weiter.

; --- 1) Verschachtelte Bloecke, jeder mit seinem eigenen Abschluss ---
For i = 1 To 2
  While i < 2
    If i = 1
      Repeat
        Select i
          Case 1
            Print "tief " + i
          Default
            Print "nie"
        End Select
      Until True
    Else
      Print "nie"
    EndIf
    i = i + 1
  Wend
Next

; --- 2) Select mit Default als letztem Teil, Case mit mehreren Werten ---
Select 3
  Case 1, 2 : Print "nie"
  Case 3, 4 : Print "drei oder vier"
  Default : Print "nie"
End Select

; --- 3) Funktionen hintereinander ---
Function Eins()
  Return 1
End Function
Function Zwei()
  Return Eins() + 1
End Function
Print "zwei " + Zwei()

; --- 4) End in einer Funktion beendet das Programm, nicht die Funktion ---
Function Beende(grund$)
  Print "ende: " + grund
  End
  Print "FEHLER: nach End in der Funktion"
End Function

Beende("aus der Funktion")
Print "FEHLER: das Programm lief nach End weiter"
