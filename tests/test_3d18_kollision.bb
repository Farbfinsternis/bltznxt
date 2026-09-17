; Kollisionen (3D-18) - Grundlagen: Methoden, Reaktionen, Trefferliste,
; EntityCollided, rekursiver EntityType, ResetEntity (BUG-154).
;
; Gemessen am Original (2026-09-17). Werte x1000 ueber Floor, damit weder die
; Rundung von Int (BUG-95) noch Str(float) (BUG-68) mitspielt.

Graphics3D 320,240,0,2
cam = CreateCamera()

Function F$(x#)
	Return Int(Floor(x * 1000.0 + 0.5))
End Function
Function P$(e)
	Return F(EntityX(e,True)) + "," + F(EntityY(e,True)) + "," + F(EntityZ(e,True))
End Function
Function Treffer$(e)
	s$ = CountCollisions(e) + ":"
	For i = 1 To CountCollisions(e)
		s = s + " [" + F(CollisionX(e,i)) + "," + F(CollisionY(e,i)) + "," + F(CollisionZ(e,i)) + " n " + F(CollisionNX(e,i)) + "," + F(CollisionNY(e,i)) + "," + F(CollisionNZ(e,i)) + " t " + F(CollisionTime(e,i)) + " tri " + CollisionTriangle(e,i) + "]"
	Next
	Return s
End Function

; --- 1) Kugel gegen Kugel, Reaktion anhalten
ClearCollisions
Collisions 1,2,1,1
a = CreatePivot() : EntityType a,1 : EntityRadius a,1
b = CreatePivot() : EntityType b,2 : EntityRadius b,2
PositionEntity b,0,0,10
UpdateWorld
PositionEntity a,0,0,20
UpdateWorld
Print "1 kugel stop: " + P(a) + " " + Treffer(a)
Print "2 ziel traegt treffer: " + CountCollisions(b)
Print "3 collided: " + (EntityCollided(a,2) = b) + " " + (EntityCollided(a,3) = 0) + " entity " + (CollisionEntity(a,1) = b)

; --- 4) weiter schieben: zweiter Anlauf traegt neu ein
PositionEntity a,0,0,20
UpdateWorld
Print "4 zweiter anlauf: " + P(a) + " " + Treffer(a)

; --- 5) Kugel gegen Kugel, gleiten
ClearCollisions
Collisions 1,2,1,2
PositionEntity a,4,0,20 : UpdateWorld
PositionEntity a,0,0,0 : UpdateWorld
Print "5 kugel gleiten: " + P(a) + " " + Treffer(a)

; --- 6) Reaktion 3 (ohne Y)
ClearCollisions
Collisions 1,2,1,3
PositionEntity a,0,6,20 : UpdateWorld
PositionEntity a,0,6,0 : UpdateWorld
Print "6 gleiten ohne y: " + P(a) + " " + Treffer(a)

; --- 7) Reaktion 0
ClearCollisions
Collisions 1,2,1,0
PositionEntity a,0,0,20 : UpdateWorld
PositionEntity a,0,0,0 : UpdateWorld
Print "7 reaktion 0: " + P(a) + " " + Treffer(a)

FreeEntity a : FreeEntity b

; --- 8) Kugel gegen Dreiecke (Wuerfel)
ClearCollisions
Collisions 1,2,2,2
k = CreatePivot() : EntityType k,1 : EntityRadius k,1
w = CreateCube() : EntityType w,2
PositionEntity w,0,0,10
UpdateWorld
PositionEntity k,0,0,20 : UpdateWorld
Print "8 wuerfel stop: " + P(k) + " " + Treffer(k)
Print "9 flaeche: " + (CollisionSurface(k,1) = GetSurface(w,1))

; --- 10) schraeg auf den Wuerfel: gleiten
PositionEntity k,0.5,0,20 : UpdateWorld
PositionEntity k,0.5,0,0 : UpdateWorld
Print "10 schraeg: " + P(k) + " " + Treffer(k)

; --- 11) auf die Kante
PositionEntity k,1.5,1.5,20 : UpdateWorld
PositionEntity k,1.5,1.5,0 : UpdateWorld
Print "11 kante: " + P(k) + " " + Treffer(k)

; --- 12) versteckte Entity kollidiert trotzdem
HideEntity w
PositionEntity k,0,0,20 : UpdateWorld
PositionEntity k,0,0,0 : UpdateWorld
Print "12 versteckt: " + P(k) + " " + Treffer(k)
ShowEntity w

; --- 13) Ellipsoid: Radius x und y verschieden
EntityRadius k,1,3
PositionEntity k,0,3.5,20 : UpdateWorld
PositionEntity k,0,3.5,0 : UpdateWorld
Print "13 ellipsoid: " + P(k) + " " + Treffer(k)
EntityRadius k,1

; --- 14) Box (Methode 3)
ClearCollisions
Collisions 1,2,3,1
EntityBox w,-2,-1,-1,4,2,2
PositionEntity k,0,0,20 : UpdateWorld
PositionEntity k,0,0,0 : UpdateWorld
Print "14 box: " + P(k) + " " + Treffer(k)

; --- 15) gedrehtes Ziel, Methode 2
ClearCollisions
Collisions 1,2,2,1
RotateEntity w,0,45,0
PositionEntity k,0,0,20 : UpdateWorld
PositionEntity k,0,0,0 : UpdateWorld
Print "15 gedreht: " + P(k) + " " + Treffer(k)
RotateEntity w,0,0,0

; --- 16) skaliertes Ziel
ScaleEntity w,2,2,2
PositionEntity k,0,0,20 : UpdateWorld
PositionEntity k,0,0,0 : UpdateWorld
Print "16 skaliert: " + P(k) + " " + Treffer(k)
ScaleEntity w,1,1,1

; --- 17) EntityType rekursiv
kind = CreateCube(w)
PositionEntity kind,0,0,-4,True
EntityType w,3,True
Print "17 rekursiv: " + (EntityCollided(k,3) = 0)
ClearCollisions
Collisions 1,3,2,1
PositionEntity k,0,0,20 : UpdateWorld
PositionEntity k,0,0,0 : UpdateWorld
Print "18 kind trifft zuerst: " + P(k) + " " + (CollisionEntity(k,1) = kind)
FreeEntity kind
EntityType w,2

; --- 19) ResetEntity
ClearCollisions
Collisions 1,2,2,1
PositionEntity k,3,4,5
RotateEntity k,10,20,30
ScaleEntity k,2,2,2
ResetEntity k
Print "19 resetentity lage: " + P(k) + " winkel " + F(EntityYaw(k))
UpdateWorld
Print "20 reset dann update: " + P(k) + " " + CountCollisions(k)
Print "21 gettype: " + GetEntityType(k) + " " + GetEntityType(w) + " " + GetEntityType(cam)

End
