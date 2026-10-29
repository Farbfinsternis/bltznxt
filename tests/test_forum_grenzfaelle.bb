; Grenzfaelle aus dem Blitz-Forum (blitzbasic.org/forum und das Archiv der
; alten Blitz3D-Fehlermeldungen auf archive.blitzbasic.org), gesammelt
; 2026-10-29. Die Installationsbeispiele treffen keinen davon. Die erwartete
; Ausgabe ist am Original gemessen (F:\dev\Blitz3D, Ausgabe per WriteFile).

; Ganzzahlige Division einer negativen Zahl durch eine Zweierpotenz: das
; Original schiebt arithmetisch und rundet damit nach unten (Archiv 88781).
Local a = -1, b = -3, c = -5
Print a/2
Print a/4
Print b/2
Print c/4
Print a/3
d = 2 : Print a/d
Print -7/2
Print (-7) Mod 2
Print a Shr 1
Print a Sar 1

; Float nach Int rundet zur naechsten geraden Zahl, Int() ebenso (Archiv 91559).
Local i%
i = 0.5 : Print i
i = 1.5 : Print i
i = 2.5 : Print i
i = -0.5 : Print i
i = -1.5 : Print i
Print Int(2.5)
Print Int(3.5)
f# = 0.6
Print (f And 1)
Print (f = 1)
Print 1280/1024

; Zwei Anweisungen ohne Trenner auf einer Zeile (Archiv 87376).
Print "a"bb=5
Print bb
x$="hi"Print x
Print Trim$(1+1) +","Trim$(1+2)

; "a=b" rechts vom "=" ist ein Vergleich (Archiv 65602).
q = 3
res=q=3
Print res

; String gegen Zahl: die Zahl wird zum String (Archiv 68850).
s$ = ""
If s <> 0 Then Print "neq" Else Print "eq"
v# = s
Print v
t$ = "0"
If t = 0 Then Print "t eq" Else Print "t neq"
u$ = "1.50"
If u = 1.5 Then Print "u eq" Else Print "u neq"
n = "12abc"
Print n

; Ein String ohne schliessendes Anfuehrungszeichen endet am Zeilenende
; (Archiv 55464).
Local us$ = "not terminated
Print us
Select us
Case "not terminated
  Print "case hit"
End Select

; Ein geloeschtes Objekt ist ueber jede Referenz Null (Archiv 97286).
a1.T = New T : a1\x = 10
b1.T = a1
Delete a1
If b1 = Null Then Print "b null" Else Print "b not null"
c1.T = New T : c1\x = 5
d1.T = Last T
Print d1\x

; Str() eines Objekts (Forum 339, Archiv 88604).
p.T = New T : p\x = 3 : p\s = "hi" : p\f = 1.5
Print Str(p)
p\o = p
Print Str(p)
p0.T = Null
Print Str(p0)

; Zahlen aus Strings (Forum 1092).
Print Float("0.1.2.3")
Print Float("1.5abc")
Print Float("  2")
Print Float("1e3")
Print Float(".5")
Print Float("abc")
Print Int("12abc")
Print Int(" 7")
Print Int("$10")
Print Int("2.9")
Print Int("-2.9")
Print Int("+5")
Print Int("99999999999")

; Schreibweisen aus dem Forum: fuehrende Null, "Step+1", If mit Klammer und
; Doppelpunkt als Block, If ohne Then, Leerzeichen vor der Dim-Klammer.
Print 010
For k = 1 To 2 Step+1
  Print k
Next
direction = 2
If( direction = 1 ) : xx = 1 : yy = 1
Else If( direction = 2 ) : xx = 2 : yy = 2
EndIf
Print xx + yy
If direction = 2 zz = 7
Print zz
Print early(0)
Dim something.player (4)
something.player (1) = New player
something (1)\mov = New animation
something (1)\mov\angle = 45
Print something (1)\mov\angle
Read r1 : Print r1

; Tag am Feld wird gelesen und verworfen (Archiv 49400, vgl. BUG-31).
p\x$ = "7"
Print p\x

; Zufallsfolgen (Archiv 69524, 101689).
SeedRnd 1
For k = 1 To 3 : Print Rand(1,100) : Next
Print Rnd(1)
SeedRnd 0
Print Rand(1,6)
Print RndSeed()

; Chr(0) ist ein gewoehnliches Zeichen (Archiv 99192).
cs$ = "ab" + Chr(0) + "c"
Print Len(cs)
Print Instr(cs, "c")

; Fliesskommaformat (Forum 378, 1593).
Print 0.0001
Print 10.0/3.0
Print 123456789.0
Print 12345678.0
Print 0.0000320119
Print 2.0/3.0
Print 999999.9
Print 0.5 * 0.0000001
Print -1.0 / 3.0

; Eigene Hilfsfunktionen mit Namen, die anderswo eingebaut sind (Forum 1447).
Print min(3, 2.5)
Print max(3, 7)
Print Lerp(0, 10, 0.5)
Print Round(2.6)
End

Data 010

Type T
  Field x
  Field s$
  Field f#
  Field o.T
End Type

Type player
  Field mov.animation
End Type

Type animation
  Field angle
End Type

Function early(m)
  If m =0 Return 5
  Return 6
End Function

Function min#(n1#, n2#)
  If n1 < n2 Then Return n1 Else Return n2
End Function

Function max#(n1#, n2#)
  If n1 > n2 Then Return n1 Else Return n2
End Function

Function Lerp#(a#, b#, t#)
  Return a + (b - a) * t
End Function

Function Round(v#)
  Return Floor(v + 0.5)
End Function
