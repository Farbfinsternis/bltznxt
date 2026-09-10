; BUG-59 und BUG-60 - feste Arrays mit eckigen Klammern, und ein Feld darf
; einen Objekttyp tragen.
;
; Referenz: die Form ist in compiler/parser.cpp eine eigene (parseVarDecl
; erzeugt einen VectorDeclNode, parseVar eine Postfix-Kette aus "\" und "["),
; nicht ein Sonderfall von Dim. VectorDeclNode::proto legt "sizes.push_back(
; n+1 )" ab - "a[n]" hat also die Indizes 0..n. Genau ein Index; die Groesse
; muss konstant sein.
;
; Alle Werte sind am laufenden Original gemessen (2026-09-10, dasselbe
; Programm mit WriteFile statt Print).

Const GROESSE = 2
Global gl[3]
Dim d(3)

Type Knoten
	Field wert
	Field feld[2]
	Field kind.Knoten[1]   ; Objekt-Tag UND Array in einer Zeile (BUG-60)
End Type

; --- Uebergabe ist eine Referenz, nicht eine Kopie ---
Local a[3]
a[0] = 10 : a[1] = 20
schreib(a)
Print "1 uebergabe a[0]=" + Str(a[0])

; --- Anfangswerte: alles genullt, wie _bbVecAlloc es mit memset tut ---
Local b[2]
Local s$[2]
Local f#[2]
Print "2 anfang int=" + Str(b[0]) + " str=[" + s[0] + "] flt=" + Str(Int(f[0]*100))

; --- a[n] hat n+1 Elemente, der letzte Index ist n ---
Local c[3]
c[3] = 77
Print "3 letztes element c[3]=" + Str(c[3])

; --- ein Const als Groesse ist zulaessig ---
Local g[GROESSE]
g[GROESSE] = 5
Print "4 const-groesse g[2]=" + Str(g[2])

; --- ein Local in einer Funktion ist bei jedem Aufruf frisch ---
zaehl() : zaehl()
Print "5 " + zaehl()

; --- Feld-Array im Type: New legt es genullt an ---
k.Knoten = New Knoten
Print "6 feld anfang=" + Str(k\feld[0])
k\feld[0] = 5 : k\feld[2] = 7
Print "6 feld gesetzt=" + Str(k\feld[0]) + "," + Str(k\feld[2])

; --- Objekt-Array im Type, mit verkettetem Zugriff ueber beide Formen ---
k\kind[0] = New Knoten
k\kind[0]\wert = 42
Print "7 verkettet=" + Str(k\kind[0]\wert)
Print "7 leer ist Null? " + Str(k\kind[1] = Null)

; --- ein Feld-Array laesst sich uebergeben ---
Print "8 summe=" + Str(summe(k\feld))

; --- Global: dieselbe Form auf Dateiebene, und ein Dim daneben stoert nicht
;     (das sind zwei verschiedene Sprachformen, siehe die Abgrenzung im
;     Buglist-Eintrag)
gl[0] = 11
d(0) = 22
Print "9 global gl[0]=" + Str(gl[0]) + " dim d(0)=" + Str(d(0))
ausFunktion()
Print "9 global nach funktion gl[1]=" + Str(gl[1])

; --- ein Array-Local hinter einem Label: der Sprung darf seine Deklaration
;     nicht ueberspringen, sonst lehnt g++ das erzeugte C++ ab (BUG-23)
Local h[2]
Goto weiter
.zurueck
Print "10 hinter label h[1]=" + Str(h[1])
Goto ende
.weiter
h[1] = 33
Goto zurueck
.ende

End

Function schreib(v[3])
	v[0] = 99
End Function

Function zaehl$()
	Local z[1]
	z[0] = z[0] + 1
	Return "zaehler=" + Str(z[0])
End Function

Function summe(v[2])
	Return v[0] + v[1] + v[2]
End Function

Function ausFunktion()
	Print "9 global in funktion gl[0]=" + Str(gl[0])
	gl[1] = 44
End Function
