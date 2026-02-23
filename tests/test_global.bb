; test_global.bb — verify Global variables are visible inside functions

Global counter% = 0
Global name$ = "world"
Global score# = 0.0

Function Increment()
    counter = counter + 1
End Function

Function AddScore(n#)
    score = score + n
End Function

Function Greet()
    Print "Hello, " + name + "!"
End Function

; --- main ---

Greet()

Increment()
Increment()
Increment()
Print "Counter: " + Str(counter)    ; expected: 3

AddScore(1.5)
AddScore(2.5)
Print "Score: " + Str(score)        ; expected: 4

; Mutate global from main, read back in function
name = "BlitzNext"
Greet()                             ; expected: Hello, BlitzNext!

Print "OK"
