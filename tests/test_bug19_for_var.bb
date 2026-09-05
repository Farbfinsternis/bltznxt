; BUG-19 — die Schleifenvariable eines For ist eine gewoehnliche Variable:
; ein gleichnamiges Global IST sie, nach der Schleife behaelt sie den Wert,
; der die Bedingung verfehlt hat, und der Rumpf darf sie weitersetzen.
; Referenz: ForNode::semant / ForNode::translate in compiler/stmtnode.cpp.

Global g% = 99
For g = 1 To 3
Next
Print "global: " + g

For neu = 1 To 3
Next
Print "implizit: " + neu

For ab = 5 To 1 Step -2
Next
Print "negativer step: " + ab

For x# = 1.0 To 2.0 Step 0.5
Next
Print "float-tag: " + x

Local durchlaeufe% = 0
For s = 1 To 10
  durchlaeufe = durchlaeufe + 1
  If s = 2 Then s = 8
Next
Print "rumpf setzt zaehler: " + s + " nach " + durchlaeufe + " durchlaeufen"

; Die Obergrenze wird bei jedem Durchlauf neu gelesen, mit und ohne Step.
Global grenze% = 5
Local ohne% = 0
For a = 1 To grenze
  ohne = ohne + 1
  If a = 2 Then grenze = 3
Next
Print "grenze ohne step: " + ohne + " durchlaeufe, a=" + a

grenze = 5
Local mit% = 0
For b = 1 To grenze Step 1
  mit = mit + 1
  If b = 2 Then grenze = 3
Next
Print "grenze mit step: " + mit + " durchlaeufe, b=" + b

Global inFunktion% = 7
Function G%()
  For inFunktion = 1 To 3
  Next
  Return inFunktion
End Function
Local r% = G()
Print "global in funktion: rueckgabe " + r + ", danach " + inFunktion
