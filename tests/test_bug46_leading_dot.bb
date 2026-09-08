; BUG-46 - Ein Float-Literal ohne fuehrende Null ist gueltiges Blitz3D.
;
; Referenz gemessen an Blitz3D 11.8 (G:\dev\Blitz3D), nicht nur nachgelesen:
; "Print .5" faltet dort zur Konstante "0.5", "Print 5." zu "5.0". Der Punkt
; wird rein lexikalisch entschieden - eine Ziffer dahinter beginnt immer eine
; Zahl, ohne Ruecksicht auf den Kontext. Belegt an zwei Ablehnungen des
; Originals: "Goto .5" scheitert mit "Expecting identifier", und "a.5" wird zu
; zwei Token ("Expecting end-of-file"). Genau ein Punkt je Zahl - "1..5" lehnt
; das Original ab, siehe neg_bug46_double_dot.bb.
;
; Deshalb bleiben Label (".weiter") und Type-Tag ("p.T") unberuehrt: dort folgt
; dem Punkt ein Buchstabe.
;
; Ganzzahlige Floatwerte sind hier bewusst vermieden. Deren Ausgabe ("1" statt
; "1.0") ist eine eigene, offene Abweichung der Stringformatierung und gehoert
; nicht zu BUG-46.

Print .5
Print -.5
Print .25 + .25

Local a# = .1
Print a

Local wert# = .75
Print wert

Print 1.5 * .5

; Label und Type-Tag muessen weiterhin gelesen werden.
Type T
  Field x
End Type
Local p.T = New T
p\x = 3
Print p\x

Goto weiter
Print "uebersprungen"
.weiter
Print "fertig"
