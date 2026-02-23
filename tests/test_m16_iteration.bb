; Milestone 16 — Type Iteration: First, Last, Before, After, Insert, Each

; ----- Setup: Type with a name field -----
Type Node
  Field val%
End Type

; Create some instances
Local a.Node = New Node
a\val = 10

Local b.Node = New Node
b\val = 20

Local c.Node = New Node
c\val = 30

; ----- Test: For Each -----
Print "=== For Each ==="
For Each n.Node
  Print n\val
Next
; Expected output: 10  20  30

; ----- Test: First / Last -----
Print "=== First / Last ==="
Local f.Node = First Node
Local l.Node = Last Node
Print f\val   ; 10
Print l\val   ; 30

; ----- Test: Before / After -----
Print "=== Before / After ==="
Local mid.Node = After First Node
Print mid\val  ; 20  (b is after a)
Local prev.Node = Before Last Node
Print prev\val ; 20  (b is before c)

; ----- Test: Insert Before -----
Print "=== Insert Before ==="
; Move c before b  → order becomes: a, c, b
Insert c Before b
For Each n.Node
  Print n\val
Next
; Expected: 10  30  20

; ----- Test: Insert After -----
Print "=== Insert After ==="
; Move a after b  → order becomes: c, b, a
Insert a After b
For Each n.Node
  Print n\val
Next
; Expected: 30  20  10

; ----- Test: Delete during iteration -----
Print "=== Delete + Each ==="
; Delete b, then iterate
Delete b
For Each n.Node
  Print n\val
Next
; Expected: 30  10

; ----- Test: First/Last after delete -----
Print "=== First/Last after delete ==="
Print (First Node)\val  ; 30
Print (Last Node)\val   ; 10

Print "DONE"
