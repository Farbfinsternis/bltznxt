; BUG-52 - ein Dim-Array von Objekten: "Dim feld.Punkt(3)".
;
; parseArrayDecl() in compiler/parser.cpp liest den Tag mit parseTypeTag(),
; Objekttypen eingeschlossen; parseVar() liest "\feld" auch hinter einem
; Arrayelement, links wie rechts vom "=". Bis 2026-09-15 scheiterte schon
; die Deklaration an "Expected '('".

Type Punkt
  Field x, y
End Type

; --- 1) Anlegen: jedes Element ist anfangs Null ---
Dim feld.Punkt(3)
Print "leer: " + (feld(0) = Null)

; --- 2) Belegen und ueber das Element auf Felder zugreifen ---
For i = 0 To 3
  feld(i) = New Punkt
  feld(i)\x = i * 10
Next
Print "feld(2)\x = " + feld(2)\x
p.Punkt = feld(1)
Print "ueber Variable: " + p\x

; --- 3) Loeschen ueber das Element ---
; Der Emitter kannte den Typ des Elements nicht und setzte nur den Eintrag
; auf Null; das Objekt blieb in der Liste.
Delete feld(0)
feld(0) = Null
n = 0
For q.Punkt = Each Punkt
  n = n + 1
Next
Print "nach Delete: " + n

; --- 4) After, Insert und Read ueber Elemente ---
r.Punkt = After feld(1)
Print "after feld(1): " + r\x
Insert feld(3) Before feld(1)
r = First Punkt
Print "erster nach Insert: " + r\x
Read feld(1)\y
Print "gelesen: " + feld(1)\y
Data 99

; --- 5) Zwei Dimensionen ---
Dim gitter.Punkt(2, 2)
gitter(1, 1) = feld(2)
Print "gitter(1,1)\x = " + gitter(1, 1)\x
