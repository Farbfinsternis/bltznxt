; BUG-95 / BUG-109 - Float -> Int rundet wie das Original zur naechsten
; Ganzzahl, bei .5 zur geraden (x87-Vorgabe): Int(), Zuweisung, Parameter,
; Return, Vorgabewert, Feld, Array, festes Array, For-Start/-Grenze/-Schritt
; und Case. Nur Read schneidet ab. For wandelt Start, Grenze und Schritt auf
; den Typ des Zaehlers, auch eine Zeichenkette (BUG-109).
; Alle Zeilen am Original gemessen (build/f2i20260918, 2026-09-18).
; Kommazahlen werden als Int(x * 10) ausgegeben, weil Str(float) noch
; abweicht (BUG-68).

Function P(s$)
	Print s
End Function

Type T
	Field a%
	Field b
	Field f#
	Field v%[2]
End Type

Function G(x%)
	Return x
End Function
Function G2(x)
	Return x
End Function
Function H%()
	Return 2.5
End Function
Function H2()
	Local f# = 3.5
	Return f
End Function
Function D1(x=1.9)
	Return x
End Function
Function D2(x%=2.5)
	Return x
End Function
Function D3(x=-1.5)
	Return x
End Function

P("int " + Int(1.9) + " " + Int(2.5) + " " + Int(3.5) + " " + Int(0.5) + " " + Int(1.5) + " " + Int(-0.5) + " " + Int(-1.5) + " " + Int(-2.5) + " " + Int(-1.9))
Local f# = 2.5
Local g# = 3.5
Local k# = -2.5
P("intvar " + Int(f) + " " + Int(g) + " " + Int(k))
Local i% = 2.5
Local j = 3.5
P("assign " + i + " " + j)
i = f : j = g
P("assignvar " + i + " " + j)
i = 7 / 2.0
P("arith " + i)
i = f * 3
P("arith2 " + i)
i = 1
i = i + 0.5
P("compound " + i)
i = 2
i = i + 0.5
P("compound2 " + i)
P("param " + G(2.5) + " " + G(3.5) + " " + G(f) + " " + G2(g) + " " + G(-2.5))
P("return " + H() + " " + H2())
P("default " + D1() + " " + D2() + " " + D3())
Local n = 0
For i = 1 To 1.9
	n = n + 1
Next
P("forto19 " + n)
n = 0
For i = 1 To 2.5
	n = n + 1
Next
P("forto25 " + n)
n = 0
For i = 1 To 3.5
	n = n + 1
Next
P("forto35 " + n)
n = 0
For i = 1 To g
	n = n + 1
Next
P("fortovar " + n)
n = 0
Local s$ = ""
For i = 0.6 To 3
	s = s + i + ","
Next
P("forstart " + s)
s = ""
For i = 2.5 To 5
	s = s + i + ","
Next
P("forstart25 " + s)
s = ""
For i = 0 To 10 Step 2.5
	s = s + i + ","
Next
P("forstep " + s)
s = ""
For i = 0 To 10 Step 3.5
	s = s + i + ","
Next
P("forstep35 " + s)
Select 2
Case 1.9
	P("case19 hit")
Default
	P("case19 miss")
End Select
Select 2
Case 2.5
	P("case25 hit")
Default
	P("case25 miss")
End Select
Select 4
Case 3.5
	P("case35 hit")
Default
	P("case35 miss")
End Select
Local nn# = 1.9
Select 2
Case nn
	P("casevar hit")
Default
	P("casevar miss")
End Select
Local sv% = 2
Select sv
Case f
	P("casevar25 hit")
Default
	P("casevar25 miss")
End Select
Select 2.5
Case 2
	P("selfloat hit")
Default
	P("selfloat miss")
End Select
Local t.T = New T
t\a = 2.5
t\b = 3.5
P("field " + t\a + " " + t\b)
Dim arr%(3)
arr(0) = 2.5
arr(1) = 3.5
arr(2) = g
P("array " + arr(0) + " " + arr(1) + " " + arr(2))
Global gl% = 2.5
P("global " + gl)
Data 2.5, 3.7, -2.5, 1.5
Local r
Read r : s = r + ","
Read r : s = s + r + ","
Read r : s = s + r + ","
Read r : s = s + r
P("read " + s)
P("intstr " + Int("2.5") + " " + Int("3.7") + " " + Int("-3.7"))
P("big " + Int(2147483648.0) + " " + Int(10000000000.0) + " " + Int(-10000000000.0))
P("chr " + Chr(65.6) + Chr(66.5) + Chr(67.5))
P("mid " + Mid("abcdefgh", 2.5, 2.5))
Local e% = 2.5 + 0
P("expr " + e)
Local h# = 0.49999997
i = h
P("almosthalf " + i)
s = ""
For i = "1" To 2
	s = s + i + ","
Next
P("forstr " + s)
s = ""
For i = 1 To "3.7"
	s = s + i + ","
Next
P("forstrto " + s)
Local x#
s = ""
For x = 0 To 1 Step 0.5
	s = s + Int(x * 10) + ","
Next
P("forfloat " + s)
s = ""
For x = 0.5 To 2
	s = s + Int(x * 10) + ","
Next
P("forfloatstart " + s)
s = ""
For i = 5 To 1 Step -1.5
	s = s + i + ","
Next
P("fornegstep " + s)
s = ""
For i = 5 To 1 Step -2.5
	s = s + i + ","
Next
P("fornegstep25 " + s)
t = New T
s = ""
For t\a = 0.6 To 2.5
	s = s + t\a + ","
Next
P("forfield " + s)
s = ""
For t\f = 0 To 1 Step 0.5
	s = s + Int(t\f * 10) + ","
Next
P("forfieldf " + s)
Dim arr2(2)
Dim farr#(2)
s = ""
For arr2(1) = 0.6 To 2.5
	s = s + arr2(1) + ","
Next
P("forarr " + s)
farr(0) = 2.5
arr2(0) = farr(0)
P("arrcopy " + arr2(0) + " " + farr(0))
Local v%[2]
v[0] = 2.5
v[1] = 3.5
P("vec " + v[0] + " " + v[1])
Local fv#[2]
fv[0] = 2
fv[1] = 3.5
P("vecf " + Int(fv[0] * 10) + " " + Int(fv[1] * 10))
t\v[0] = 2.5
t\v[1] = 3.5
P("fieldvec " + t\v[0] + " " + t\v[1])
s = ""
Local kk% = 7
kk = kk / 2
P("intdiv " + kk)
kk = 7
kk = kk * 0.5
P("mulhalf " + kk)
P("sgnint " + Int(Sgn(-2.5)))
Local st2$ = 2.5
P("fstr " + st2)
Data 3.7, 3.7, 3.7
Read arr2(2)
Read t\a
Read v[1]
P("readziel " + arr2(2) + " " + t\a + " " + v[1])
