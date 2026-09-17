; BUG-149 - Weltdrehung ohne Skalierung der Eltern (Entity::getWorldRotation /
; setWorldRotation): EntityPitch/Yaw/Roll global, RotateEntity global,
; PointEntity, AlignToVector.
;
; Gemessen am Original (2026-09-17). Werte x100 ueber Floor, damit weder
; die Rundung von Int (BUG-95) noch Str(float) (BUG-68) mitspielt.

Function F$(x#)
	Return Int(Floor(x * 100.0 + 0.5))
End Function
Function W$(e)
	Return F(EntityPitch(e)) + "," + F(EntityYaw(e)) + "," + F(EntityRoll(e)) + " welt " + F(EntityPitch(e,True)) + "," + F(EntityYaw(e,True)) + "," + F(EntityRoll(e,True))
End Function

opa = CreatePivot()
RotateEntity opa,15,0,-25
ScaleEntity opa,1,3,0.5
par = CreatePivot(opa)
RotateEntity par,0,90,20
ScaleEntity par,2,1,3
PositionEntity par,1,2,3

; 1) Abfrage
k = CreatePivot(par)
RotateEntity k,40,0,30
Print "1 abfrage: " + W(k)

; 2) nur der Elternteil skaliert, gleichmaessig
p2 = CreatePivot()
RotateEntity p2,0,90,20
ScaleEntity p2,2,2,2
k2 = CreatePivot(p2)
RotateEntity k2,0,0,30
Print "2 gleichmaessig: " + W(k2)

; 3) RotateEntity global
k3 = CreatePivot(par)
RotateEntity k3,10,20,30,True
Print "3 rotate global: " + W(k3)

; 4) TurnEntity global
k4 = CreatePivot(par)
RotateEntity k4,5,0,0
TurnEntity k4,0,30,0,True
Print "4 turn global: " + W(k4)

; 5) PointEntity
ziel = CreatePivot()
PositionEntity ziel,10,-4,7
k5 = CreatePivot(par)
PointEntity k5,ziel
Print "5 point: " + W(k5)
PointEntity k5,ziel,33
Print "6 point roll 33: " + W(k5)

; 7) PointEntity ohne Elternteil als Gegenprobe
k7 = CreatePivot()
PositionEntity k7,1,1,1
PointEntity k7,ziel,10
Print "7 point ohne eltern: " + W(k7)

; 8) AlignToVector
k8 = CreatePivot(par)
RotateEntity k8,10,0,0
AlignToVector k8,1,1,0,2
Print "8 align y: " + W(k8)
k9 = CreatePivot(par)
AlignToVector k9,0,-1,1,3,0.5
Print "9 align z halb: " + W(k9)
End
