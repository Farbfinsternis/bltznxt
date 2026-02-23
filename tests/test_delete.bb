; test_delete.bb — verify Delete First/Last and proper list cleanup

Type Node
    Field val%
End Type

; Create 4 nodes in order: 10, 20, 30, 40
Local a.Node = New Node : a\val = 10
Local b.Node = New Node : b\val = 20
Local c.Node = New Node : c\val = 30
Local d.Node = New Node : d\val = 40

; Count: should be 4
Local count% = 0
For Each n.Node
    count = count + 1
Next
Print "Count: " + Str(count)           ; expected: 4

; --- Delete First ---
Delete First Node    ; removes a (val=10)

count = 0
For Each n.Node
    count = count + 1
Next
Print "After Del First: " + Str(count) ; expected: 3

; New first should be b (val=20)
Local f.Node = First Node
Print "New first: " + Str(f\val)       ; expected: 20

; --- Delete Last ---
Delete Last Node     ; removes d (val=40)

count = 0
For Each n.Node
    count = count + 1
Next
Print "After Del Last: " + Str(count)  ; expected: 2

; New last should be c (val=30)
Local l.Node = Last Node
Print "New last: " + Str(l\val)        ; expected: 30

; --- Delete simple variable ---
Delete b             ; removes b (val=20)

count = 0
For Each n.Node
    count = count + 1
Next
Print "After Del b: " + Str(count)     ; expected: 1

; Only c (val=30) should remain
Local only.Node = First Node
Print "Only: " + Str(only\val)         ; expected: 30

; --- Delete inside For Each (deletion-safe pattern) ---
; Add fresh nodes
Local x.Node = New Node : x\val = 1
Local y.Node = New Node : y\val = 2
Local z.Node = New Node : z\val = 3

; Delete all nodes in For Each loop
For Each n.Node
    Delete n
Next

count = 0
For Each n.Node
    count = count + 1
Next
Print "After Each Delete: " + Str(count)  ; expected: 0

Print "OK"
