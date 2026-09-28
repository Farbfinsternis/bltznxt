; 3D-Klang - ein zweiter Listener bricht ab ("Listener already created",
; im Original im Debug-Modus; wir pruefen immer). Nach FreeEntity darf ein
; neuer entstehen.

Graphics3D 320,240,0,2
lis = CreateListener(0)
FreeEntity lis
lis = CreateListener(0, 0.5)
Print "neu " + EntityClass(lis)
lis2 = CreateListener(0)
Print "nicht erreicht"
End
