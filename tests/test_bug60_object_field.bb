; BUG-60 - ein Feld darf einen Objekttyp tragen, und was hinter dem Tag auf
; der Zeile steht, gehoert dazu.
;
; Referenz: parseVarDecl liest im Type-Rumpf denselben parseTypeTag() wie
; ueberall, und der kennt neben %, # und $ auch ".Ident" (compiler/parser.cpp).
; Unser Type-Rumpf las bis hierher nur die drei skalaren Tags und uebersprang
; alles andere ohne Diagnose - "Field child.T" wurde still ein int, und ein
; "Field child.T,x" verlor das x gleich mit.
;
; Der verkettete Zugriff gehoert dazu: ohne ihn faellt der falsche Feldtyp
; gar nicht auf. Er scheiterte vorher an derselben Stelle wie BUG-59, weil
; der Statement-Parser nur EINE Feldebene kannte.
;
; Werte am laufenden Original gemessen (2026-09-10).

Type T
	Field child.T,x        ; Objekt-Tag mit einem weiteren Feld dahinter
	Field v
	Field name$
End Type

p.T = New T
p\x = 7
p\name = "wurzel"

p\child = New T
p\child\v = 9
p\child\name = "kind"

; drei Ebenen, damit die Kette wirklich eine Kette ist
p\child\child = New T
p\child\child\v = 11

Print "x=" + Str(p\x)
Print "name=" + p\name
Print "kind v=" + Str(p\child\v)
Print "kind name=" + p\child\name
Print "enkel v=" + Str(p\child\child\v)
Print "summe=" + Str(p\x + p\child\v + p\child\child\v)
Print "leeres feld ist Null? " + Str(p\child\child\child = Null)
