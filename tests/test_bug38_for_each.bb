; BUG-38 - "For <var> = Each <Type>" ist die Blitz3D-Schreibweise.
; Referenz: compiler/parser.cpp, case FOR - erst parseVar(), dann '=',
; dann EACH. Der Type-Tag am Zaehler ist optional, weil parseVar() ihn
; ueber parseTypeTag() liest; ForEachNode::semant verlangt nur, dass der
; Zaehler ein Objekt genau dieses Typs ist.

Type Punkt
  Field x%
End Type

Type Marke
  Field name$
End Type

Local a.Punkt = New Punkt : a\x = 1
Local b.Punkt = New Punkt : b\x = 2
Local c.Punkt = New Punkt : c\x = 3

Local m1.Marke = New Marke : m1\name = "eins"
Local m2.Marke = New Marke : m2\name = "zwei"

; --- mit Type-Tag am Zaehler ---
Print "mit Tag:"
For p.Punkt = Each Punkt
  Print p\x
Next

; --- ohne Tag, aber der Zaehler ist vorher deklariert ---
; Ohne Tag UND ohne Deklaration waere er ein int - das lehnt Blitz3D ab
; und wir seit 2026-09-07 auch, siehe neg_bug38_each_var_type.bb.
Local q.Punkt
Print "ohne Tag:"
For q = Each Punkt
  Print q\x
Next

; --- der Zaehler war schon da und hat den passenden Typ ---
Local r.Punkt
Print "vorher deklariert:"
For r = Each Punkt
  Print r\x
Next

; --- verschachtelt ueber zwei Listen ---
Print "verschachtelt:"
For p2.Punkt = Each Punkt
  For mm.Marke = Each Marke
    Print p2\x
    Print mm\name
  Next
Next

; --- Loeschen waehrend des Laufs bleibt sicher ---
Print "mit Delete:"
For d.Punkt = Each Punkt
  If d\x = 2 Then Delete d
Next
For e.Punkt = Each Punkt
  Print e\x
Next

Print "DONE"
