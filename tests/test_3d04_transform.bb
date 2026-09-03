; 3D-04: Transform System & Scene Graph
; Tests: PositionEntity, RotateEntity, ScaleEntity, EntityX/Y/Z, UpdateWorld,
;        MoveEntity, TurnEntity, EntityDistance, EntityPitch/Yaw/Roll, ResetEntity

; Simple text-mode test — no Graphics3D required because the transform math
; operates purely on the entity struct (no GL needed for data-level tests).

Graphics3D 800,600,32,1

Local p = CreatePivot()

; --- PositionEntity + EntityX/Y/Z (local, glob=0) ---
PositionEntity p, 1.0, 2.0, 3.0
Print EntityX(p)     ; 1
Print EntityY(p)     ; 2
Print EntityZ(p)     ; 3

; --- UpdateWorld propagates to world matrix ---
UpdateWorld
Print EntityX(p, 1)  ; 1 (world = local when no parent)
Print EntityY(p, 1)  ; 2
Print EntityZ(p, 1)  ; 3

; --- RotateEntity (local) ---
RotateEntity p, 10.0, 20.0, 30.0
Print EntityPitch(p) ; 10
Print EntityYaw(p)   ; 20
Print EntityRoll(p)  ; 30

; --- ScaleEntity ---
ScaleEntity p, 2.0, 2.0, 2.0
UpdateWorld
; After scale the position should still be 1,2,3 (scale doesn't change position)
Print EntityX(p, 1)  ; 1
Print EntityY(p, 1)  ; 2
Print EntityZ(p, 1)  ; 3

; --- ResetEntity ---
ResetEntity p
UpdateWorld
Print EntityX(p, 1)  ; 0
Print EntityY(p, 1)  ; 0
Print EntityZ(p, 1)  ; 0

; --- EntityDistance ---
Local a = CreatePivot()
Local b = CreatePivot()
PositionEntity a, 0.0, 0.0, 0.0
PositionEntity b, 3.0, 4.0, 0.0
UpdateWorld
Print EntityDistance(a, b)   ; 5

FreeEntity p
FreeEntity a
FreeEntity b
Print "OK"
