; Comprehensive Test for Custom Types (Structs)
Type Monster
    Field name$
    Field hp
End Type

DebugLog "--- Type Full Test ---"

; 1. Creation and New
m1.Monster = New Monster : m1\name = "Goblin" : m1\hp = 50
m2.Monster = New Monster : m2\name = "Orc" : m2\hp = 100
m3.Monster = New Monster : m3\name = "Dragon" : m3\hp = 500

DebugLog "Created 3 monsters."

; 2. Iteration (Each)
DebugLog "Iterating with Each:"
For m.Monster = Each Monster
    DebugLog "  - " + m\name + " (HP: " + m\hp + ")"
Next

; 3. Collections (First, Last, After, Before)
firstM.Monster = First Monster
lastM.Monster = Last Monster
afterM.Monster = After firstM
beforeM.Monster = Before lastM

DebugLog "First: " + firstM\name
DebugLog "Last: " + lastM\name
DebugLog "After First: " + afterM\name
DebugLog "Before Last: " + beforeM\name

; 4. Insert (Recently added)
; Move Dragon (Last) to First
Insert m3 Before firstM

DebugLog "After Insert Dragon Before Goblin:"
For m.Monster = Each Monster
    DebugLog "  - " + m\name
Next

; 5. Delete and Lifecycle
Delete m2 ; Delete Orc
DebugLog "After deleting Orc:"
For m.Monster = Each Monster
    DebugLog "  - " + m\name
Next

; 6. Delete Each
Delete Each Monster
DebugLog "After Delete Each Monster:"
count = 0
For m.Monster = Each Monster
    count = count + 1
Next
DebugLog "Count: " + count

DebugLog "--- End Type Full Test ---"
