; Milestone 9: Const Declarations

Const MaxHP% = 100
Const Kreis# = 3.14159   ; nicht 'Pi': das ist ein reserviertes Wort (BUG-35)
Const Greeting$ = "Hello"
Const A% = 10, B% = 20

Print MaxHP
Print Kreis
Print Greeting
Print A + B

; Const in expression
If MaxHP > 50 Then Print "HP is high"
