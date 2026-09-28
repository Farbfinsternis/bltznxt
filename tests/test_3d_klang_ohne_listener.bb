; 3D-Klang - EmitSound ohne Listener bricht ab ("No Listener created",
; im Original im Debug-Modus; wir pruefen immer). Auch ein freigegebener
; Listener zaehlt nicht mehr.

Graphics3D 320,240,0,2
lis = CreateListener(0)
FreeEntity lis
p = CreatePivot()
Print "vorher"
EmitSound 0, p
Print "nicht erreicht"
End
