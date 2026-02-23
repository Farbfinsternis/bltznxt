; Milestone 14+15: Type Instances — New, Delete, Field Access (\)

; --- Basic struct + field write/read ---
Type Vec
  Field x%, y%
End Type

Local v.Vec = New Vec
v\x = 10
v\y = 20
Print v\x + v\y      ; -> 30

; --- Float fields ---
Type Point
  Field px!, py!
End Type

Local p.Point = New Point
p\px = 1.5
p\py = 2.5
Print p\px + p\py    ; -> 4 (float addition, 4.000000 in cout default)

; --- Multiple instances ---
Type Counter
  Field n%
End Type

Local a.Counter = New Counter
Local b.Counter = New Counter
a\n = 5
b\n = 7
Print a\n + b\n      ; -> 12

; --- Roadmap test: Delete ---
Type Vec2 : Field x! : End Type
Local v2.Vec2 = New Vec2
v2\x = 3.0
Print v2\x           ; -> 3
Delete v2

Print "M14+M15 OK"
