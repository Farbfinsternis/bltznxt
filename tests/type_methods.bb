Type Point
    Field x, y
    
    Method Init(nx, ny)
        Self\x = nx
        Self\y = ny
    End Method
    
    Method PrintCoords()
        Print "Point Coords: " + Self\x + ", " + Self\y
    End Method
    
    Method GetSum()
        Return Self\x + Self\y
    End Method
End Type

p.Point = New Point
p\Init(10, 20)
p\PrintCoords()

s = p\GetSum()
Print "Sum: " + s

p\x = 100
p\y = 200
p\PrintCoords()
Print "New Sum: " + p\GetSum()
