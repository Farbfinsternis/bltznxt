; BUG-18 - "Select" ohne einen einzigen "Case" ist gueltiges Blitz3D.
; Der SELECT-Zweig von parseStmtSeq liest DEFAULT direkt hinter dem
; Ausdruck, und SelectNode::translate gibt den Default-Rumpf hinter den
; Vergleichen unbedingt aus - gibt es keinen Vergleich, laeuft er immer.
; Wir erzeugten dafuer ein "else" ohne "if"; das C++ uebersetzte nicht.

Global zaehler% = 0

Function Tick%()
  zaehler = zaehler + 1
  Return zaehler
End Function

Local a% = 7

; 1) nur Default
Select a
  Default
    Print "1: nur Default"
End Select

; 2) Cases und Default, kein Treffer
Select a
  Case 1
    Print "2: falsch"
  Case 2
    Print "2: falsch"
  Default
    Print "2: Default"
End Select

; 3) Cases und Default, Treffer - der Default bleibt aussen vor
Select a
  Case 1
    Print "3: falsch"
  Case 7
    Print "3: Case 7"
  Default
    Print "3: falsch"
End Select

; 4) ueberhaupt kein Zweig
Select a
End Select
Print "4: nach leerem Select"

; 5) der Ausdruck wird auch ohne Case ausgewertet, und genau einmal
Select Tick()
  Default
    Print "5: Default"
End Select
Print "5: Zaehler nach einem Durchlauf:"
Print zaehler

; 6) derselbe Fall mit einem String
Local s$ = "abc"
Select s
  Default
    Print "6: nur Default mit String"
End Select

; 7) verschachtelt - ein nur-Default im Rumpf eines Case
Select a
  Case 7
    Select a
      Default
        Print "7: inneres Default"
    End Select
End Select

Print "DONE"
