; BUG-57 - "Else If" ist dasselbe wie "ElseIf" - aber nur bei genau einem
; Leerzeichen.
;
; Referenz gemessen an Blitz3D 11.8 (G:\dev\Blitz3D), nicht nur nachgelesen.
; Der Zwischenraum ist hier **bedeutungstragend**, nicht bloss eine Frage der
; Strenge:
;
;   "Else If"    (ein Leerzeichen)  -> ein Token ELSEIF, braucht *ein* EndIf
;   "Else  If"   (zwei Leerzeichen) -> Else + verschachteltes If, braucht *zwei*
;   "Else<TAB>If"                   -> ebenso verschachtelt
;   "Else" <NL> "If"                -> ebenso verschachtelt
;
; Alle vier Faelle sind am Original gemessen. Dieselbe Ein-Leerzeichen-Regel
; gilt fuer die vier "End X"-Abschluesse: "End  If" und "End<TAB>If" lehnt das
; Original ab. Vorher fassten wir bei beliebigem Zwischenraum derselben Zeile
; zusammen und nahmen damit Programme an, die die Referenz zurueckweist.

a = 2

; --- 1) Else If mit einem Leerzeichen: eine Kette, ein EndIf
If a = 1
  Print "eins"
Else If a = 2
  Print "zwei"
Else
  Print "andere"
EndIf

; --- 2) laengere Kette, gemischt geschrieben
b = 3
If b = 1
  Print "b1"
ElseIf b = 2
  Print "b2"
Else If b = 3
  Print "b3"
EndIf

; --- 3) zwei Leerzeichen sind ein verschachteltes If und brauchen ein eigenes
;        EndIf - genau deshalb ist die Regel keine Kosmetik
If a = 1
  Print "nicht"
Else  If a = 2
  Print "verschachtelt-zwei-leerzeichen"
  EndIf
EndIf

; --- 4) die vier End-Formen mit einem Leerzeichen bleiben unberuehrt
If a = 2
  Print "endif-form"
End If

Type T
  Field v
End Type

Select a
  Case 2
    Print "select-form"
End Select

F()

Function F()
  Print "function-form"
End Function
