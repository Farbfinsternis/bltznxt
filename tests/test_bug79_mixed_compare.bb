; BUG-79 - ein Vergleich zwischen Zahl und Zeichenkette wird als Zeichenkette
; entschieden, nicht als Zahl.
;
; Referenz: RelExprNode::semant in compiler/exprnode.cpp bestimmt einen
; gemeinsamen Vergleichstyp und castet BEIDE Seiten darauf - ist eine Seite
; ein String, wird als String verglichen; sonst als Float, wenn eine Seite
; Float ist; sonst als Int. Dieselbe Stufenfolge wie bei der Arithmetik.
;
; Alle erwarteten Werte sind am laufenden Original gemessen (2026-09-10,
; dasselbe Programm mit WriteLine statt Print). Die Richtung ist der Kern des
; Befundes: 9 < "10" ist FALSCH, obwohl 9 < 10 wahr ist.
;
; NICHT hier geprueft: Vergleiche, deren Ergebnis von der Schreibweise eines
; Floats abhaengt - 1.0 = "1" ist im Original falsch (es schreibt "1.0"), bei
; uns wahr. Das ist BUG-68 und wuerde hier als Sollverhalten festgeschrieben.

; --- gemischt: Zahl gegen Zeichenkette, beide Reihenfolgen, alle sechs
;     Operatoren. Verglichen wird "9" gegen "10", nicht 9 gegen 10.
Print "K  9 <  '10'   = " + (9 < "10")
Print "K '9' >  10    = " + ("9" > 10)
Print "K '9' <  10    = " + ("9" < 10)
Print "K  10 >= '9'   = " + (10 >= "9")
Print "K '9' <= 9     = " + ("9" <= 9)
Print "K '9' >= 10    = " + ("9" >= 10)
Print "K '9' <> 10    = " + ("9" <> 10)
Print "K  10 =  '10'  = " + (10 = "10")
Print "K  10 =  '10.0'= " + (10 = "10.0")
Print "K '1' <> 1     = " + ("1" <> 1)
Print "K  0  =  ''    = " + (0 = "")

; --- gemischt mit Float. Umgewandelt wird wie beim Verketten, nicht gerundet.
Print "K  1.5 = '1.5' = " + (1.5 = "1.5")
Print "K  1.5 = '1.50'= " + (1.5 = "1.50")
Print "K  2.0 < '12'  = " + (2.0 < "12")
Print "K  1.5 < '2'   = " + (1.5 < "2")
Print "K '2' > 1.5    = " + ("2" > 1.5)

; --- reine Zahlen bleiben ein Zahlenvergleich.
Print "K  10 <  9     = " + (10 < 9)
Print "K  2  <  1.5   = " + (2 < 1.5)
Print "K  1.5 < 2     = " + (1.5 < 2)

; --- reine Zeichenketten: zeichenweise, Gross und Klein unterschieden,
;     und vorzeichenlos gerechnet (Chr(200) steht hinter "A", nicht davor).
Print "K 'abc' < 'abd'= " + ("abc" < "abd")
Print "K 'A' < 'a'    = " + ("A" < "a")
Print "K 'a' < 'B'    = " + ("a" < "B")
Print "K ''  < 'a'    = " + ("" < "a")
Print "K 'A' = 'A'    = " + ("A" = "A")
Print "K Chr(200)>'A' = " + (Chr(200) > "A")

; --- derselbe Weg ueber Variablen, also der Laufzeitpfad statt der
;     Konstantenfaltung des Originals.
s$ = "9" : i% = 10 : f# = 1.5 : n% = -5
Print "V  s <  i      = " + (s < i)
Print "V  i <  s      = " + (i < s)
Print "V  s >  i      = " + (s > i)
Print "V  i =  s      = " + (i = s)
Print "V  f <  s      = " + (f < s)
Print "V  n =  '-5'   = " + (n = "-5")
Print "V  n <  '-40'  = " + (n < "-40")

; --- die Form, an der es aufgefallen ist (KBSplines.bb): eine gelesene Zahl
;     gegen eine Zeichenkette gehalten.
Print "V Int('1')<>'1'= " + (Int("1") <> "1")

; --- Eine Schleifengrenze ist KEIN Vergleich in diesem Sinn, obwohl der
;     Emitter dafuer "<=" schreibt: sie wird zur Zahl gewandelt, und zwar zur
;     Kommazahl. "To 3.7" liefe dagegen viermal - eine konstante Kommazahl
;     wird auf den Zaehlertyp gerundet, ein gewandelter String nicht.
;     Ohne diese Unterscheidung waere aus einem lauten Uebersetzungsfehler
;     ein stilles Falschergebnis geworden: "To '10'" einmal statt zehnmal.
z = 0
For i = 1 To "10"
  z = z + 1
Next
Print "S For 1 To '10'  = " + z
z = 0
For i = 1 To "3.7"
  z = z + 1
Next
Print "S For 1 To '3.7' = " + z
z = 0
For i = 1 To 10 Step "3"
  z = z + 1
Next
Print "S For Step '3'   = " + z

; --- Ein Case wandelt seinen Wert auf den Typ des Select-Ausdrucks, nicht
;     auf einen gemeinsamen Typ. "Select 10 : Case '010'" trifft deshalb,
;     "Select '9.0' : Case 9" nicht - und "Case '1.7'" auf 1 trifft, weil
;     abgeschnitten und nicht gerundet wird.
c1$ = "9"
Select c1
  Case 9
    Print "C str '9'  vs 9     = trifft"
  Default
    Print "C str '9'  vs 9     = Default"
End Select
c2 = 10
Select c2
  Case "010"
    Print "C int 10   vs '010' = trifft"
  Default
    Print "C int 10   vs '010' = Default"
End Select
c3$ = "9.0"
Select c3
  Case 9
    Print "C str '9.0' vs 9    = trifft"
  Default
    Print "C str '9.0' vs 9    = Default"
End Select
c4 = 1
Select c4
  Case "1.7"
    Print "C int 1    vs '1.7' = trifft"
  Default
    Print "C int 1    vs '1.7' = Default"
End Select
c5# = 1.5
Select c5
  Case "1.5"
    Print "C flt 1.5  vs '1.5' = trifft"
  Default
    Print "C flt 1.5  vs '1.5' = Default"
End Select
c6$ = "abc"
Select c6
  Case 0
    Print "C str 'abc' vs 0    = trifft"
  Default
    Print "C str 'abc' vs 0    = Default"
End Select
