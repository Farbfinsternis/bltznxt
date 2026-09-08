; BUG-47 - Ein Type-Tag an einer Zuweisung deklariert die Variable.
;
; Referenz gemessen an Blitz3D 11.8 (G:\dev\Blitz3D), nicht nur nachgelesen.
; Das Original liest jede Variable ueber parseVar()/parseTypeTag(), das Tag ist
; also ueberall erlaubt, wo ein skalares Tag erlaubt ist - auch vor dem
; Feldtrenner ("p.T\v = 1" wird angenommen).
;
; Entscheidend und leicht falsch zu raten: **das Tag oeffnet keinen neuen
; Gueltigkeitsbereich.** Es liefert nur den Typ, wenn die Variable noch nicht
; existiert; sonst bindet der Name wie gewoehnlich. Im Assembler des Originals
; schreibt "p.T = New T" innerhalb einer Funktion in das globale Slot (_vp),
; wenn ein "Global p.T" existiert - es entsteht dort keine lokale Variable.
; Genau das prueft F() weiter unten. Ohne passendes Global entsteht dagegen
; eine lokale ([ebp-N] im Original), siehe G().
;
; Dieselbe Regel gilt seit BUG-38 bereits am For-Zaehler.

Type T
  Field v
End Type

; 1) Deklaration durch Zuweisung, danach gewoehnlicher Feldzugriff
p.T = New T
p\v = 7
Print p\v

; 2) Dasselbe Tag noch einmal ist zulaessig, ebenso der spaetere Verzicht darauf
p.T = New T
p\v = 8
p = New T
p\v = 9
Print p\v

; 3) Tag vor dem Feldtrenner
p.T\v = 11
Print p\v

; 4) Delete muss den Typ kennen, den erst die Zuweisung eingefuehrt hat
q.T = New T
Delete q
Print "geloescht"

; 5) Das Tag bindet an das vorhandene Global, es verdeckt es nicht
Global g.T = New T
g\v = 1
F()
Print g\v

; 6) Ohne passendes Global entsteht eine lokale Variable
G()

Function F()
  g.T = New T
  g\v = 42
End Function

Function G()
  lokal.T = New T
  lokal\v = 5
  Print lokal\v
End Function
