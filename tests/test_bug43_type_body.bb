; BUG-43 - die Gegenprobe: was im Type-Rumpf gueltig bleibt.
;
; Nachgeschlagen in compiler/parser.cpp, parseStructDecl() (2026-09-15):
; zwischen Name, Field-Zeilen und End Type ueberspringt die Referenz
; Zeilenenden - leere Zeilen und Kommentarzeilen also auch -, aber keinen
; Doppelpunkt. Die Ablehnungsfaelle liegen als neg_bug43_* daneben.

; --- 1) Leerzeilen und Kommentare zwischen den Feldern ---
Type Punkt

  ; die Lage
  Field x#, y#

  Field name$   ; ein Kommentar hinter dem Feld

End Type

p.Punkt = New Punkt
p\x = 1.5
p\y = 2.5
p\name = "p"
Print p\name + " " + (p\x + p\y)

; --- 2) Das erste Field auf der Zeile des Namens ---
; Nach dem Namen muessen keine Zeilenenden kommen; die Schleife prueft nur,
; ob FIELD folgt. Gelesen, nicht am Original gemessen.
Type Kiste Field inhalt
End Type

k.Kiste = New Kiste
k\inhalt = 42
Print "kiste " + k\inhalt

; --- 3) Mehrere Typen hintereinander, einer bezieht sich auf den anderen ---
Type Knoten
  Field wert
  Field naechster.Knoten
End Type

a.Knoten = New Knoten
b.Knoten = New Knoten
a\wert = 1
b\wert = 2
a\naechster = b
Print "kette " + a\wert + " " + a\naechster\wert
