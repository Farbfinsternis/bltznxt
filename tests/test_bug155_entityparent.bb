; EntityParent: was bleibt stehen, die lokale oder die Weltlage?
;
; Gemessen am Original (2026-09-17). Werte x1000 ueber Floor.

Graphics3D 320,240,0,2
Function F$(x#)
	Return Int(Floor(x * 1000.0 + 0.5))
End Function
Function Z$(e)
	Return "lokal " + F(EntityX(e)) + "," + F(EntityY(e)) + "," + F(EntityZ(e)) + " / " + F(EntityPitch(e)) + "," + F(EntityYaw(e)) + "," + F(EntityRoll(e)) + " welt " + F(EntityX(e,True)) + "," + F(EntityY(e,True)) + "," + F(EntityZ(e,True)) + " / " + F(EntityPitch(e,True)) + "," + F(EntityYaw(e,True)) + "," + F(EntityRoll(e,True))
End Function

p = CreatePivot()
PositionEntity p,0,-1,0
k = CreatePivot()
PositionEntity k,0,0,0
EntityParent k,p
UpdateWorld
Print "1 ohne flag: " + Z(k)

p2 = CreatePivot()
PositionEntity p2,0,-1,0
k2 = CreatePivot()
EntityParent k2,p2,True
UpdateWorld
Print "2 mit flag True: " + Z(k2)

p3 = CreatePivot()
PositionEntity p3,0,-1,0
k3 = CreatePivot()
EntityParent k3,p3,False
UpdateWorld
Print "3 mit flag False: " + Z(k3)

; gedrehter, skalierter Elternteil
p4 = CreatePivot()
PositionEntity p4,1,2,3
RotateEntity p4,0,90,20
ScaleEntity p4,2,1,3
k4 = CreatePivot()
PositionEntity k4,4,5,6
RotateEntity k4,10,20,30
EntityParent k4,p4
UpdateWorld
Print "4 gedreht ohne flag: " + Z(k4)

p5 = CreatePivot()
PositionEntity p5,1,2,3
RotateEntity p5,0,90,20
ScaleEntity p5,2,1,3
k5 = CreatePivot()
PositionEntity k5,4,5,6
RotateEntity k5,10,20,30
EntityParent k5,p5,True
UpdateWorld
Print "5 gedreht mit flag: " + Z(k5)

; abhaengen
EntityParent k4,0
UpdateWorld
Print "6 abhaengen ohne flag: " + Z(k4)
EntityParent k5,0,True
UpdateWorld
Print "7 abhaengen mit flag: " + Z(k5)

; derselbe Elternteil noch einmal
EntityParent k,p
UpdateWorld
Print "8 gleicher elternteil: " + Z(k)
End
