; WEAK-14 Stufe 2, zweiter Teil: derselbe Vorschlag auch fuer einen Typnamen
; und fuer einen Feldnamen. Der letzte Feldname ist absichtlich weit weg -
; dort darf kein Vorschlag stehen.

Type Spieler
  Field name$
  Field punkte%
End Type

Local p.Spieler = New Spielr
p\punkt = 5
Print p\vollkommenanders
