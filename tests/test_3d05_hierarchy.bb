; 3D-05: Entity Hierarchy
; Tests: EntityParent, GetParent, CountChildren, GetChild, FindChild,
;        EntityOrder, EntityClass

Graphics3D 800,600,32,1

Local root   = CreatePivot()
Local child1 = CreatePivot()
Local child2 = CreatePivot()

NameEntity child1, "Alpha"
NameEntity child2, "Beta"

; --- EntityParent / GetParent / CountChildren ---
EntityParent child1, root
EntityParent child2, root

Print CountChildren(root)      ; 2
Print (GetParent(child1) = root)  ; 1 (True)
Print (GetParent(child2) = root)  ; 1 (True)

; --- GetChild (1-based) ---
Print (GetChild(root, 1) = child1)  ; 1
Print (GetChild(root, 2) = child2)  ; 1
Print GetChild(root, 3)             ; 0 (out of range)

; --- FindChild (recursive) ---
Local grandchild = CreatePivot()
NameEntity grandchild, "Gamma"
EntityParent grandchild, child1

Print (FindChild(root, "Alpha") = child1)      ; 1
Print (FindChild(root, "Gamma") = grandchild)  ; 1 (deep search)
Print FindChild(root, "NoSuch")                ; 0

; --- EntityOrder ---
EntityOrder child1, 10
; no crash expected

; --- EntityClass ---
Print EntityClass(root)    ; Pivot

; --- Re-parent to null (detach) ---
EntityParent child2, 0
Print GetParent(child2)       ; 0
Print CountChildren(root)     ; 1 (only child1 remains)

; --- FreeEntity parent cascades to children ---
FreeEntity root   ; also frees child1 and grandchild

Print "OK"
