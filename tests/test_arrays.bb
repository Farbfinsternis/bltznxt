; Milestone 10: Array Indexing

; 1D integer array
Dim arr%(5)
arr(0) = 10
arr(1) = 20
arr(2) = 42
Print arr(2)
Print arr(0) + arr(1)

; 1D float array
Dim scores#(3)
scores(0) = 1.5
scores(1) = 2.5
Print scores(0) + scores(1)

; 2D array
Dim grid%(3,3)
grid(1,1) = 99
grid(2,0) = 7
Print grid(1,1)
Print grid(2,0)

; Array in loop
Dim vals%(4)
For i% = 0 To 4
  vals(i) = i * 10
Next i
Print vals(3)
