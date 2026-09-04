; BUG-23 — Goto/Gosub darf ueber Deklarationen springen. C++ verbietet einen
; Sprung, der die Initialisierung einer Variablen ueberspringt; Blitz3D kennt
; diese Einschraenkung nicht. Deklarationen in Rumpfen mit Labels werden daher
; vorgezogen, die Deklaration an Ort und Stelle wird zur Zuweisung.

Type Node
  Field Val%
End Type

Gosub Setup

; Alles hier steht zwischen Sprung und Label
Local s$ = "string nach gosub"
Local n.Node = New Node
n\Val = 42
Local zahl% = 3

Print s
Print n\Val
Print zahl
Print counter

Goto Ende

.Setup
counter = 5
Print "setup gelaufen"
Return

.Ende
Local fin$ = "ende"
Print fin

; Deklaration in einem Block, danach ein Label
If 1 = 1 Then
  Local drin$ = "aus block"
End If
Print drin

Goto Wirklich

.NichtErreicht
Print "nicht erreicht"

.Wirklich
Print "fertig"
