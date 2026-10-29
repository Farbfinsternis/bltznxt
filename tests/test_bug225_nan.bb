; BUG-225 - Vergleiche mit NaN wie im Original.
;
; Das Original vergleicht mit dem x87; ein ungeordneter Vergleich liest sich
; dort als "gleich" und als "groesser". Ist eine Seite NaN, sind "=", ">" und
; ">=" wahr, "<>", "<" und "<=" falsch, Not ist 1 und Sgn -1. C++ folgt IEEE
; 754 (nur "<>" wahr). NaN entsteht in alten Spielen vor allem bei
; ACos(skalarprodukt) knapp ueber 1; danach nahm das Programm bei uns andere
; Zweige. Gefunden im Archiv der Blitz3D-Fehlermeldungen (Thema 57799).
; Die erwartete Ausgabe ist die des Originals (gemessen 2026-10-29).

n# = Sqr(-1)
o# = 1
i% = 5
Print n
Print "= " + (n = 0) + (n = 1) + (n = n) + (o = n)
Print "<> " + (n <> 0) + (n <> n) + (o <> n)
Print "< " + (n < 0) + (n < 1) + (o < n)
Print "> " + (n > 0) + (n > 1) + (o > n)
Print "<= " + (n <= 0) + (o <= n)
Print ">= " + (n >= 0) + (o >= n)
Print "int: " + (i = n) + (i <> n) + (i < n) + (i > n) + (i <= n) + (i >= n)
Print "nint: " + (n = i) + (n <> i) + (n < i) + (n > i) + (n <= i) + (n >= i)
Print "asin: " + (ASin(2) = 0) + (ASin(2) <> 0) + (ASin(2) < 0) + (ASin(2) > 0) + (ASin(2) <= 0) + (ASin(2) >= 0)
Print "acos: " + (ACos(1.5) = 1.0) + (ACos(1.5) > 1.0) + (1.0 > ACos(1.5)) + (1.0 = ACos(1.5))
s# = Sin(30) * n
Print "ext: " + (s = 0) + (s > 0) + (s < 0) + (Sin(30) * n = 0) + (Sin(30) * n > 0) + (Sin(30) * n < 0)
Print "inf: " + (1.0/0.0001 > 0) + (n = 1.0/0.0001)
If n Then Print "if n" Else Print "if not n"
If Not n Then Print "not n" Else Print "not not n"
If n = 0 And n = 1 Then Print "both"
b = (n > 0) : Print "b=" + b

Select n
Case 1 : Print "select case 1"
Case 0 : Print "select case 0"
Default : Print "select default"
End Select
Select 3
Case n : Print "select3 case n"
Default : Print "select3 default"
End Select

c = 0 : While n <> 0 And c < 3 : c = c + 1 : Wend : Print "while n<>0: " + c
c = 0 : Repeat : c = c + 1 : Until n > 100 Or c = 3 : Print "until n>100: " + c
m# = 0
For k = 1 To 3
  If n > m Then m = n
Next
Print "max " + m

Print "sgn " + Sgn(n) + " " + Sgn(-n) + " " + Sgn(ASin(2)) + " " + Sgn(0.0) + " " + Sgn(-2.5)
Print "neg " + (-n = 0) + (-n > 0)
Print ACos(1.5)
Print Int(n)

; Ohne NaN bleibt alles wie vorher: Gleichheit von Float und Int ueber Float.
f# = 16777217
Print (f = 16777216) + " " + (f = 16777217) + " " + (0.1 + 0.2 = 0.3) + " " + (Sin(30) = 0.5)
