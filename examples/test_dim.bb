; Test for Arrays (Dim)

Dim myArr(10)
myArr(0) = 5
myArr(10) = 15

DebugLog "myArr(0): " + myArr(0)
DebugLog "myArr(10): " + myArr(10)

; Multi-dimensional
Dim matrix(3, 3)
matrix(1, 2) = 42
DebugLog "matrix(1, 2): " + matrix(1, 2)

; Array in function
TestArray()

Function TestArray()
	DebugLog "Inside function - myArr(1): " + myArr(1)
	myArr(1) = 123
	DebugLog "Inside function - updated myArr(1): " + myArr(1)
End Function

DebugLog "Back in Main - myArr(1): " + myArr(1)
