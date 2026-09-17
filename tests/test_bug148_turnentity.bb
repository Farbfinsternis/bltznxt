; BUG-148 - TurnEntity dreht um die eigenen Achsen (lokal: Lage * Delta)
; bzw. um die Weltachsen (global: Delta * Weltlage, zurueck in den Raum des
; Elternteils).
;
; Gemessen am Original (2026-09-17). Werte x1000 ueber Floor, damit weder
; die Rundung von Int (BUG-95) noch Str(float) (BUG-68) mitspielt.
; Nicht hier: Weltwinkel unter skaliertem Elternteil (BUG-149) und das
; Vorzeichen von Roll/Yaw bei 180 Grad (BUG-150).

Function F$(x#)
	Return Int(Floor(x * 1000.0 + 0.5))
End Function
Function F2$(x#)
	Return Int(Floor(x * 100.0 + 0.5))
End Function
Function W$(e)
	Return F(EntityPitch(e)) + "," + F(EntityYaw(e)) + "," + F(EntityRoll(e)) + " welt " + F(EntityPitch(e,True)) + "," + F(EntityYaw(e,True)) + "," + F(EntityRoll(e,True))
End Function
Function P$(e)
	MoveEntity e,0,0,1
	Return F(EntityX(e,True)) + "," + F(EntityY(e,True)) + "," + F(EntityZ(e,True)) + " / " + W(e)
End Function

; 1) Rollen, dann Nicken
p = CreatePivot()
TurnEntity p,0,0,45
TurnEntity p,30,0,0
Print "1 roll45 dann pitch30: " + P(p)

; 2) ein Aufruf wirkt wie RotateEntity
q = CreatePivot()
TurnEntity q,30,0,45
Print "2 turn 30,0,45: " + P(q)

; 3) zehnmal nicken und rollen
r = CreatePivot()
For i = 1 To 10
	TurnEntity r,3,0,6
Next
Print "3 zehnmal 3,0,6: " + P(r)

; 4) Gier nach Nicken: lokal
a = CreatePivot()
TurnEntity a,45,0,0
TurnEntity a,0,90,0
Print "4 pitch45 dann yaw90 lokal: " + P(a)

; 5) dasselbe global
b = CreatePivot()
TurnEntity b,45,0,0
TurnEntity b,0,90,0,True
Print "5 pitch45 dann yaw90 global: " + P(b)

; 6) Kind eines gedrehten Pivots, lokal und global
par = CreatePivot()
RotateEntity par,0,90,20
k1 = CreatePivot(par)
TurnEntity k1,0,0,30
TurnEntity k1,40,0,0
; zwei Stellen: die dritte liegt hier auf der Rundungsgrenze (BUG-150)
Print "6 kind lokal: " + F2(EntityPitch(k1)) + "," + F2(EntityYaw(k1)) + "," + F2(EntityRoll(k1)) + " welt " + F2(EntityPitch(k1,True)) + "," + F2(EntityYaw(k1,True)) + "," + F2(EntityRoll(k1,True))
k2 = CreatePivot(par)
TurnEntity k2,0,0,30
TurnEntity k2,40,0,0,True
Print "7 kind global: " + W(k2)

; 8) ueber die Senkrechte nicken
c = CreatePivot()
For i = 1 To 5
	TurnEntity c,40,0,0
Next
MoveEntity c,0,0,1
Print "8 fuenfmal pitch40: " + F(EntityX(c)) + "," + F(EntityY(c)) + "," + F(EntityZ(c)) + " / " + F(EntityPitch(c))

; 9) viele kleine Drehungen wie im Jet-Demo
d = CreatePivot()
For i = 1 To 600
	TurnEntity d,1.7,0,-2.3
	MoveEntity d,0,0,0.5
Next
Print "9 600 schritte: " + F(EntityX(d)) + "," + F(EntityY(d)) + "," + F(EntityZ(d)) + " / " + W(d)

; 10) Gegenprobe: reine Gierdrehungen summieren sich
g = CreatePivot()
TurnEntity g,0,30,0
TurnEntity g,0,50,0
TurnEntity g,0,50,0,True
Print "10 gier 30+50+50: " + P(g)
End
