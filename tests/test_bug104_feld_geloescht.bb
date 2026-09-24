; BUG-104 - Feldzugriff ueber eine Referenz auf ein geloeschtes Objekt.
;
; Wir pruefen immer wie der Debug-Modus des Originals (FieldVarNode::translate)
; und melden "Object does not exist". Ohne Debug-Modus stuerzt das Original
; hier ab; bei uns las der Zugriff bis BUG-104 freigegebenen Speicher.
; Meldung gemessen am 2026-09-24 (build/obj20260924/feld_zombie), sie steht
; in .expected_stderr.

Type T
Field x
End Type
p.T=New T
p\x=5
q.T=p
Print "vorher: "+q\x
Delete p
Print "vor dem Zugriff"
Print "geloescht: "+q\x
Print "nicht erreicht"
End
