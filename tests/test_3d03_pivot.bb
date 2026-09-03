; 3D-03: Entity Handle System & Pivot
; Tests: CreatePivot, HideEntity, ShowEntity, NameEntity, EntityName$, FreeEntity
Graphics3D 800,600,32,1

; Basic create / free
Local p1 = CreatePivot()
Print p1             ; expect non-zero handle

; Name setter / getter
NameEntity p1, "MyPivot"
Print EntityName(p1) ; expect MyPivot

; Visibility toggle (no crash expected)
HideEntity p1
ShowEntity p1

; Child pivot parented to p1
Local p2 = CreatePivot(p1)
Print p2             ; expect a different handle

; FreeEntity frees parent and child — must not crash
FreeEntity p1

; Create a fresh pivot after free
Local p3 = CreatePivot()
Print p3             ; expect valid handle again

FreeEntity p3
Print "OK"
