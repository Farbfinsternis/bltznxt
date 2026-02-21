Type Player
    Field x, y
    Field name$
End Type

DebugLog "--- Type Test ---"

p.Player = New Player
p\x = 10
p\y = 20
p\name = "Antigravity"

DebugLog "Player: " + p\name + " at (" + p\x + ", " + p\y + ")"

p2.Player = New Player
p2\x = 100
p2\y = 200
p2\name = "BltzNext"

DebugLog "Iterating with Each:"
For p.Player = Each Player
    DebugLog "Found Player: " + p\name + " (x=" + p\x + ")"
Next

Delete p2

DebugLog "After delete p2:"
For p.Player = Each Player
    DebugLog "Found Player: " + p\name + " (x=" + p\x + ")"
Next

DebugLog "--- End Type Test ---"
