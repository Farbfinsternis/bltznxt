; Picking (3D-17): EntityPickMode mit allen drei Methoden, LinePick mit und
; ohne Radius, EntityPick, PickedX/NX/Time/Entity/Surface/Triangle,
; EntityVisible mit und ohne Obscurer.
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

w = CreateCube()
PositionEntity w,0,0,10
UpdateWorld
; --- 21) Picking
EntityPickMode w,2
PositionEntity w,0,0,10 : UpdateWorld
p = LinePick(0,0,0,0,0,20)
Print "21 linepick: " + (p = w) + " " + F(PickedX()) + "," + F(PickedY()) + "," + F(PickedZ()) + " n " + F(PickedNX()) + "," + F(PickedNY()) + "," + F(PickedNZ()) + " t " + F(PickedTime()) + " tri " + PickedTriangle() + " flaeche " + (PickedSurface() = GetSurface(w,1))
p = LinePick(0,5,0,0,0,20)
Print "22 daneben: " + p + " t " + F(PickedTime())
p = LinePick(0,2.5,0,0,0,20,2)
Print "23 mit radius: " + (p = w) + " " + F(PickedX()) + "," + F(PickedY()) + "," + F(PickedZ()) + " t " + F(PickedTime())

; --- 24) Pickmodus 1 (Kugel) und 3 (Box)
EntityPickMode w,1
EntityRadius w,3
p = LinePick(0,0,0,0,0,20)
Print "24 pick kugel: " + (p = w) + " t " + F(PickedTime()) + " n " + F(PickedNZ())
EntityPickMode w,3
EntityBox w,-1,-1,-1,2,2,2
p = LinePick(0,0,0,0,0,20)
Print "25 pick box: " + (p = w) + " t " + F(PickedTime())
EntityPickMode w,0
p = LinePick(0,0,0,0,0,20)
Print "26 pick aus: " + p

; --- 27) EntityPick und EntityVisible
EntityPickMode w,2
q = CreatePivot()
PositionEntity q,0,0,0
PointEntity q,w
UpdateWorld
p = EntityPick(q,20)
Print "27 entitypick: " + (p = w) + " t " + F(PickedTime())
ziel = CreatePivot()
PositionEntity ziel,0,0,20 : UpdateWorld
Print "28 sichtbar ohne obscurer: " + EntityVisible(q,ziel)
EntityPickMode w,2,1
Print "29 obscurer dazwischen: " + EntityVisible(q,ziel)
EntityPickMode w,2,0
Print "30 obscurer aus: " + EntityVisible(q,ziel) + " pick " + (LinePick(0,0,0,0,0,20) = w)
End
