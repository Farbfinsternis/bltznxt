; BUG-224 - Das Objekt-Tag an einer Variablen in einem Ausdruck.
;
; Gemessen an Blitz3D (F:\dev\Blitz3D): parsePrimary() der Referenz liest
; jeden Bezeichner ueber parseTypeTag(), das Tag ".T" ist also nicht nur links
; vom "=" erlaubt (BUG-47), sondern ueberall, wo eine Variable steht - auch
; an einem Feldnamen ("p\nxt.T\x", dort wie bei BUG-31 gelesen und verworfen).
; Alte Programme schreiben das Tag oft konsequent mit; vorher scheiterten sie
; an jeder solchen Zeile mit "unexpected token '\'".
; Die erwartete Ausgabe ist die des Originals.

Type T
  Field x
  Field s$
  Field nxt.T
End Type

Global g.T = New T
g.T\x = 7
Dim arr.T(3)
Local p.T = New T
p.T\x = 3 : p.T\s = "abc"
Local q.T = New T
q\x = 4
arr.T(1) = q.T
p\nxt = q
p\nxt.T\x = 40

Print p.T\x + 1
Print p.T\s + "!"
Print g.T\x
Print arr.T(1)\x
Print p.T\nxt.T\x
If p.T = q.T Then Print "same" Else Print "diff"
Print f(p.T)
Print f(q)
Insert q.T Before p.T
ff.T = First T : Print ff.T\x
h = Handle(q.T)
oo.T = Object.T(h) : Print oo.T\x
c = 0
o.T = First T
While o.T <> Null
  c = c + o.T\x
  o = After o.T
Wend
Print c
For e.T = Each T
  Print e.T\x
Next

; Eine noch unbekannte Variable mit Tag im Ausdruck entsteht mit dem Typ des
; Tags, nicht als Integer - im Hauptprogramm wie in einer Funktion.
If z.T = Null Then Print "z null"
z = New T
z\x = 5
Print z\x
Print lazy()
End

Function f(a.T)
  Return a.T\x * 2
End Function

Function lazy()
  If w.T = Null Then w = New T : w\x = 9
  Return w\x
End Function
