; BUG-74, die Gegenrichtung: Formen, bei denen ein Feldzugriff GUELTIG ist und
; die neue Diagnose deshalb schweigen muss. Jede einzelne ist am laufenden
; Original geprueft - es nimmt dieses Programm an und liefert dieselben Werte.
;
; NICHT hier drin: "c = New Knoten" auf eine Variable ohne Tag. Das Original
; lehnt es mit "Illegal type conversion" ab (gemessen) - eine implizite
; Variable ist dort ein Int und kann kein Objekt halten. Wir nehmen es an und
; merken uns sogar den Typ; das ist ein eigener Befund und gehoert nicht in
; einen Test, der sonst mit dem Original uebereinstimmt.
;
; Der Sinn dieses Tests ist nicht die Ausgabe, sondern das Ausbleiben einer
; Meldung: die Pruefung in fieldType() darf nur zuschlagen, wenn der Typ des
; Traegers BEKANNT und kein Objekttyp ist. Ein unbekannter Typ bleibt still.

Type Knoten
  Field wert
  Field kind.Knoten
End Type

Function Summe(k.Knoten)
  ; Objektparameter - der Traeger ist hier ein Parameter, keine Deklaration
  Return k\wert
End Function

Global a.Knoten = New Knoten
a\wert = 7
Global b.Knoten = New Knoten
b\wert = 5

; Kette ueber ein Objektfeld (BUG-60)
a\kind = b
Print "kette      = " + a\kind\wert

; Objektparameter
Print "parameter  = " + Summe(a)

; Typ kommt aus First
d.Knoten = First Knoten
Print "aus First  = " + d\wert

; For Each
s = 0
For e.Knoten = Each Knoten
  s = s + e\wert
Next
Print "for each   = " + s

; Festes Array aus Objekten, mit Index
Local v.Knoten[1]
v[0] = a
v[1] = b
Print "array[0]   = " + v[0]\wert
Print "array[1]   = " + v[1]\wert
