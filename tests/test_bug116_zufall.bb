; BUG-116 - Zufallsgenerator wie im Original (bbmath.cpp): Park-Miller mit
; Faktor 48271, Startzustand $1234, Rnd(from,to=0), Rand(from,to=1).
;
; Gemessen am Original (2026-09-17). Float-Werte gehen ueber Floor, damit
; weder die Rundung von Int (BUG-95) noch Str(float) (BUG-68) mitspielt.

Function F$(x#)
	Return Int(Floor(x * 10000.0))
End Function

Print "1 start " + RndSeed()
Print "2 rnd4 " + F(Rnd(4)) + " " + F(Rnd(4)) + " " + F(Rnd(4))
Print "3 seed " + RndSeed()

SeedRnd 0
Print "4 seed0 " + RndSeed()
SeedRnd -1
Print "5 seedm1 " + RndSeed()
SeedRnd $80000000
Print "6 seedmin " + RndSeed()
SeedRnd 1
Print "7 rand100 " + Rand(100) + " seed " + RndSeed()
SeedRnd 1
Print "8 randm10 " + Rand(-10)

SeedRnd 7
s$ = ""
For i = 1 To 10
	s = s + Int(Floor(Rnd(1) * 65536)) + " "
Next
Print "9 rnd1 " + s

SeedRnd 99
Print "10 rnd " + F(Rnd(100)) + " " + F(Rnd(10,20)) + " " + F(Rnd(20,10)) + " " + F(Rnd(-5)) + " " + F(Rnd(0)) + " " + F(Rnd(3,3))

SeedRnd 42
s = ""
For i = 1 To 10
	s = s + Rand(6) + " "
Next
Print "11 rand6 " + s
s = ""
For i = 1 To 10
	s = s + Rand(3,9) + "/" + Rand(9,3) + " "
Next
Print "12 rand39 " + s
Print "13 rand0 " + Rand(0) + " " + Rand(1) + " " + Rand(-1,-1) + " " + Rand(5,5)

SeedRnd 12345
s = ""
For i = 1 To 10
	s = s + Rand(1,100000000) + " "
Next
Print "14 randgross " + s
s = ""
For i = 1 To 6
	s = s + Rand(0,2147483647) + " " + Rand(-2147483647,2147483647) + " "
Next
Print "15 randueberlauf " + s
s = ""
For i = 1 To 6
	x# = Rnd(1000000)
	s = s + Int(Floor(x)) + " " + Int(Floor(Rnd(-3.5,1000000000.0))) + " "
Next
Print "16 rndgross " + s
Print "17 seed " + RndSeed()

; Folge ueber viele Aufrufe
SeedRnd 2026
sum = 0
For i = 1 To 100000
	sum = sum + Rand(1000)
Next
Print "18 summe " + sum + " seed " + RndSeed()
End
