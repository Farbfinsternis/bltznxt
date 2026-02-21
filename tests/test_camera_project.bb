; Test CameraProject
Graphics3D 800, 600, 0, 2
SetBuffer BackBuffer()

piv = CreatePivot()
PositionEntity piv, 10, 5, 20

cam = CreateCamera()
PositionEntity cam, 0, 5, 0

; Project the pivot
CameraProject cam, EntityX(piv), EntityY(piv), EntityZ(piv)

px# = ProjectedX()
py# = ProjectedY()
pz# = ProjectedZ()

Print "Projected: " + px + ", " + py + ", " + pz

; Expected: 
; Pivot is at (10, 5, 20). Camera at (0, 5, 0).
; Relative pos: (10, 0, 20).
; Screen center X (400) + offset.
; Since it's directly to the right (X=10) and forward (Z=20).
; In perspective, it should be to the right of center.
; Y is 0 relative to cam, so it should be at vertical center (300).

If px > 400 Then Print "ProjectedX OK (Right of center)" Else Print "ProjectedX FAIL"
If Abs(py - 300) < 1.0 Then Print "ProjectedY OK (Center)" Else Print "ProjectedY FAIL"
If pz > 0 Then Print "ProjectedZ OK (>0)" Else Print "ProjectedZ FAIL"

End
