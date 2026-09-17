; BUG-145 - TranslateEntity verschiebt im Raum des Elternteils, ohne die
; eigene Drehung; MoveEntity dagegen entlang der eigenen Achsen.
;
; Gemessen am Original (2026-09-17). Ausgabe in Hundertsteln, abgeschnitten.

Graphics3D 320,240,0,2

Function P$(e)
	Return Int(EntityX(e,True)*100) + "," + Int(EntityY(e,True)*100) + "," + Int(EntityZ(e,True)*100)
End Function

; 1) Gier 90, lokal verschieben: Drehung wirkt nicht
p = CreatePivot()
RotateEntity p,0,90,0
TranslateEntity p,0,0,1
Print "1 gier 90 translate 0,0,1: " + P(p)

; 2) danach global: wie ohne Elternteil
TranslateEntity p,1,0,0,True
Print "2 global 1,0,0: " + P(p)

; 3) Nick 30, Gier 45
q = CreatePivot()
RotateEntity q,30,45,0
TranslateEntity q,0,1,0
Print "3 nick 30 gier 45 translate 0,1,0: " + P(q)

; 4) Kind eines gedrehten Pivots: dessen Drehung wirkt
par = CreatePivot()
RotateEntity par,0,90,0
k = CreatePivot(par)
TranslateEntity k,0,0,1
Print "4 kind von gier 90 translate 0,0,1: " + P(k)

; 5) Kamera nach PointEntity (Jet Tails)
cam = CreateCamera()
PositionEntity cam,0,1,3
t = CreatePivot()
PositionEntity t,10,0,0
PointEntity cam,t
TranslateEntity cam,0,0,-5
Print "5 pointentity dann translate 0,0,-5: " + P(cam)

; 6) Gegenprobe: MoveEntity folgt der eigenen Drehung
m = CreatePivot()
RotateEntity m,0,90,0
MoveEntity m,0,0,1
Print "6 gier 90 move 0,0,1: " + P(m)

; 7) Gegenprobe: TranslateEntity ohne Drehung und Rollen
r = CreatePivot()
RotateEntity r,0,0,90
TranslateEntity r,2,0,0
TranslateEntity r,0,-3,0
Print "7 rollen 90 translate 2,0,0 und 0,-3,0: " + P(r)
End
