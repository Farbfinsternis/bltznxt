; BUG-16 — Blitz3D hat keinen Blockgueltigkeitsbereich. IfNode::semant,
; WhileNode::semant, ForNode::semant und StmtSeqNode::semant reichen alle
; dasselbe Environ weiter, und IdentVarNode::semant traegt eine implizit
; erzeugte Variable in die Deklarationsliste des ganzen Rumpfes ein.
; Alles hier drin war vorher "was not declared in this scope".

Local a% = 1
If a = 1 Then
  imIf = 5
EndIf
Print "im If erzeugt: " + imIf

While a < 3
  imWhile = a * 10
  a = a + 1
Wend
Print "im While erzeugt: " + imWhile

For p = 1 To 2
  For q = 1 To 2
  Next
Next
Print "innere Schleifenvariable: " + q

Repeat
  imRepeat$ = "hallo"
Until 1
Print "im Repeat erzeugt: " + imRepeat

Select a
  Case 3
    imCase# = 1.5
End Select
Print "im Case erzeugt: " + imCase

; Ein Local ohne Initialisierer wird in Blitz3D nicht bei jedem Durchlauf
; zurueckgesetzt: VarDeclNode::translate erzeugt nur dann Code, wenn ein
; Initialisierer da ist.
For runde = 1 To 3
  Local zaehler%
  zaehler = zaehler + 1
Next
Print "Local ohne Initialisierer haelt: " + zaehler

; Dasselbe in einer Funktion, damit der Rumpf-Pfad mitgeprueft ist.
Function Pruefe%()
  Local n% = 0
  If n = 0 Then
    innen = 42
  EndIf
  Return innen
End Function
Print "in Funktion: " + Pruefe()
