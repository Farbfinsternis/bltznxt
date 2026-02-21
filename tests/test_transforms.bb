; test_transforms.bb - Verifying hierarchy and transformation system
Graphics3D 800, 600, 0, 2

; 1. Basic Hierarchy Test
parent = CreatePivot()
PositionEntity parent, 10, 0, 0

child = CreatePivot(parent)
PositionEntity child, 5, 0, 0 ; Parent-relative

DebugLog "--- Basic Hierarchy ---"
DebugLog "Parent X: " + EntityX(parent)
DebugLog "Child Local X: " + EntityX(child, 0)
DebugLog "Child World X (Expected 15): " + EntityX(child, 1)

; 2. Global Position Test
PositionEntity child, 0, 0, 0, 1 ; Move to world origin
DebugLog "--- Global Position ---"
DebugLog "Child World X (Expected 0): " + EntityX(child, 1)
DebugLog "Child Local X (Expected -10): " + EntityX(child, 0)

; 3. Rotation Test
RotateEntity parent, 0, 90, 0 ; Face left (world Z axis relative to parent)
PositionEntity child, 1, 0, 0, 0 ; Offset 1 unit along parent's local X (which is world -Z now? No, parent's local X points to world +Z)
; World X should be parent.x (10), World Z should be -1
DebugLog "--- Rotation ---"
DebugLog "Child World X (Expected 10): " + EntityX(child, 1)
DebugLog "Child World Z (Expected -1): " + EntityZ(child, 1)

; 4. TurnEntity Test
RotateEntity parent, 0, 0, 0
PositionEntity child, 1, 0, 0, 0
TurnEntity parent, 0, 90, 0, 0 ; Relative turn
DebugLog "--- TurnEntity ---"
DebugLog "Parent Yaw (Expected 90): " + EntityYaw(parent)
DebugLog "Child World Z (Expected -1): " + EntityZ(child, 1)

; 5. TranslateEntity Test
PositionEntity parent, 0, 0, 0
PositionEntity child, 0, 0, 0
TranslateEntity child, 10, 5, 0, 1 ; World displacement
DebugLog "--- TranslateEntity ---"
DebugLog "Child World X (Expected 10): " + EntityX(child, 1)
DebugLog "Child World Y (Expected 5): " + EntityY(child, 1)

; 6. RotateEntity Global Test
RotateEntity parent, 0, 45, 0
RotateEntity child, 0, 0, 0, 1 ; World rotation 0
DebugLog "--- RotateEntity Global ---"
DebugLog "Child World Yaw (Expected 0): " + EntityYaw(child, 1)
DebugLog "Child Local Yaw (Expected -45): " + EntityYaw(child, 0)

; 7. MoveEntity Test
PositionEntity child, 0, 0, 0, 0
RotateEntity child, 0, 90, 0
MoveEntity child, 0, 0, 5 ; Move 5 units along local Z (which is world +X in B3D/this engine)
DebugLog "--- MoveEntity ---"
DebugLog "Child Local X (Expected 5): " + EntityX(child, 0)

WaitKey
End
