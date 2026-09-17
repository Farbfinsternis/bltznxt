; Kollisionen, zweite Messreihe: jeder Fall wird ueber ResetEntity sauber
; gestellt (Start setzen, ResetEntity, Ziel setzen, UpdateWorld).
;
; Gemessen am Original (2026-09-17). Werte x1000 ueber Floor.

Graphics3D 320,240,0,2
cam = CreateCamera()

Function F$(x#)
	Return Int(Floor(x * 1000.0 + 0.5))
End Function
Function Lauf$(e,x0#,y0#,z0#,x1#,y1#,z1#)
	PositionEntity e,x0,y0,z0
	ResetEntity e
	PositionEntity e,x1,y1,z1
	UpdateWorld
	s$ = F(EntityX(e,True)) + "," + F(EntityY(e,True)) + "," + F(EntityZ(e,True)) + " | " + CountCollisions(e)
	For i = 1 To CountCollisions(e)
		s = s + " [" + F(CollisionX(e,i)) + "," + F(CollisionY(e,i)) + "," + F(CollisionZ(e,i)) + " n " + F(CollisionNX(e,i)) + "," + F(CollisionNY(e,i)) + "," + F(CollisionNZ(e,i)) + " t " + F(CollisionTime(e,i)) + " tri " + CollisionTriangle(e,i) + "]"
	Next
	Return s
End Function

; Ziel: eine Wand aus einem Wuerfel bei z=10, 10 breit, 10 hoch, 1 tief
wand = CreateCube()
ScaleEntity wand,5,5,0.5
PositionEntity wand,0,0,10
EntityType wand,2
ball = CreatePivot()
EntityType ball,1
EntityRadius ball,1
UpdateWorld

ClearCollisions
Collisions 1,2,2,1
Print "1 frontal stop: " + Lauf(ball,0,0,0,0,0,20)
Print "2 knapp daneben: " + Lauf(ball,7,0,0,7,0,20)
Print "3 schraeg von links: " + Lauf(ball,-5,0,0,5,0,20)
ClearCollisions
Collisions 1,2,2,2
Print "4 gleiten: " + Lauf(ball,-5,0,0,5,0,20)
Print "5 gleiten flach: " + Lauf(ball,0,0,0,20,0,20)
Print "6 gleiten von oben: " + Lauf(ball,0,8,0,0,-8,20)
ClearCollisions
Collisions 1,2,2,3
Print "7 gleiten ohne y: " + Lauf(ball,0,8,0,0,-8,20)

; Ecke: zwei Waende im rechten Winkel
wand2 = CreateCube()
ScaleEntity wand2,0.5,5,5
PositionEntity wand2,6,0,5
EntityType wand2,2
UpdateWorld
ClearCollisions
Collisions 1,2,2,2
Print "8 in die ecke: " + Lauf(ball,0,0,0,10,0,20)
Print "9 genau in die kante: " + Lauf(ball,0,0,0,6,0,10)
FreeEntity wand2

; Kante und Ecke des Wuerfels
ClearCollisions
Collisions 1,2,2,1
Print "10 auf die obere kante: " + Lauf(ball,0,6,0,0,5,20)
Print "11 auf die ecke: " + Lauf(ball,6,6,0,5,5,20)

; Methode 1 (Kugel) und 3 (Box) gegen dieselbe Wand
ClearCollisions
Collisions 1,2,1,1
Print "12 methode kugel: " + Lauf(ball,0,0,0,0,0,20)
ClearCollisions
Collisions 1,2,3,1
Print "13 methode box: " + Lauf(ball,0,0,0,0,0,20)
Print "14 box schraeg: " + Lauf(ball,-5,0,0,5,0,20)

; Ellipsoid
ClearCollisions
Collisions 1,2,2,1
EntityRadius ball,1,3
Print "15 ellipsoid frontal: " + Lauf(ball,0,0,0,0,0,20)
Print "16 ellipsoid ueber die kante: " + Lauf(ball,0,6,0,0,4,20)
EntityRadius ball,1

; gedrehtes und skaliertes Ziel
RotateEntity wand,0,30,0
UpdateWorld
Print "17 ziel gedreht: " + Lauf(ball,0,0,0,0,0,20)
RotateEntity wand,0,0,0
ScaleEntity wand,2,5,0.5
UpdateWorld
Print "18 ziel breiter: " + Lauf(ball,0,0,0,0,0,20)
ScaleEntity wand,5,5,0.5
UpdateWorld

; mehrere Ziele, welches gewinnt
naeher = CreateCube()
PositionEntity naeher,0,0,5
EntityType naeher,2
UpdateWorld
Print "19 zwei ziele: " + Lauf(ball,0,0,0,0,0,20) + " welches " + (CollisionEntity(ball,1) = naeher)
FreeEntity naeher

; Kugel gegen eine Kugel aus Dreiecken
kugel = CreateSphere(8)
PositionEntity kugel,0,0,10
EntityType kugel,3
UpdateWorld
ClearCollisions
Collisions 1,3,2,1
Print "20 gegen kugelnetz: " + Lauf(ball,0,0,0,0,0,20)
Print "21 kugelnetz schraeg: " + Lauf(ball,0.5,0.5,0,0.5,0.5,20)
FreeEntity kugel

; Ziel bewegt sich mit
ClearCollisions
Collisions 1,2,2,1
PositionEntity wand,0,0,10
UpdateWorld
Print "22 langsam vorwaerts: " + Lauf(ball,0,0,8,0,0,9)
Print "23 startet schon drin: " + Lauf(ball,0,0,9.6,0,0,9.8)
Print "24 rueckwaerts heraus: " + Lauf(ball,0,0,9.6,0,0,0)

; viele Schritte gegen die Wand
ClearCollisions
Collisions 1,2,2,2
PositionEntity ball,-9,0,0
ResetEntity ball
s$ = ""
For i = 1 To 12
	TranslateEntity ball,1.5,0,1.5
	UpdateWorld
	s = s + F(EntityX(ball,True)) + "/" + F(EntityZ(ball,True)) + " "
Next
Print "25 zwoelf schritte: " + s
End
