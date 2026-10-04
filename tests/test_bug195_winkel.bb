; BUG-195 (Gruppe 2) - VectorYaw, VectorPitch, DeltaYaw, DeltaPitch, GetMatElement.
;
; Nach bbblitz3d.cpp und geom.h des Originals (Vector::yaw = -atan2(x,z),
; pitch = -atan2(y, sqrt(x*x+z*z))); alle Werte am Original gemessen
; (2026-10-03), Winkel in 1/10000 Grad, Matrix in 1/10000.

Graphics3D 320, 240, 32, 2

Function F$(x#)
	Return Int(Floor(x * 10000 + 0.5))
End Function
Function V(x#, y#, z#)
	Print "vec " + x + "," + y + "," + z + ": yaw " + F(VectorYaw(x, y, z)) + " pitch " + F(VectorPitch(x, y, z))
End Function
V(0, 0, 1) : V(1, 0, 0) : V(-1, 0, 0) : V(0, 0, -1) : V(1, 1, 1) : V(0, 1, 0) : V(0, -1, 0) : V(0, 0, 0) : V(-3, 2, -0.5)
a = CreatePivot() : b = CreatePivot()
Function D(name$, a, b)
	Print name + ": dyaw " + F(DeltaYaw(a, b)) + " dpitch " + F(DeltaPitch(a, b))
End Function
PositionEntity b, 0, 0, 10 : D("vorn", a, b)
PositionEntity b, 10, 0, 0 : D("rechts", a, b)
PositionEntity b, 0, 0, -10 : D("hinten", a, b)
PositionEntity b, -10, 5, -10 : D("links oben hinten", a, b)
RotateEntity a, 20, 170, 0 : D("gedreht", a, b)
RotateEntity a, -30, -170, 15 : PositionEntity b, 3, -4, 5 : D("gedreht 2", a, b)
PositionEntity b, 0, 0, 0 : PositionEntity a, 0, 0, 0 : D("gleicher ort", a, b)
e = CreateCube() : PositionEntity e, 1, 2, 3 : RotateEntity e, 30, 45, 60 : ScaleEntity e, 2, 3, 4
s$ = ""
For r = 0 To 3 : For c = 0 To 2
	s = s + F(GetMatElement(e, r, c)) + " "
Next : Next
Print "matrix " + s
k = CreatePivot(e) : PositionEntity k, 1, 0, 0 : RotateEntity k, 0, 90, 0
s = ""
For r = 0 To 3 : For c = 0 To 2
	s = s + F(GetMatElement(k, r, c)) + " "
Next : Next
Print "kind " + s
End
