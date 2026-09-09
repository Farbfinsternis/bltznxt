; BUG-64 - ein Bezeichner vor einem Doppelpunkt ist ein Aufruf, keine
; Sprungmarke.
;
; Bis 2026-09-09 hat der Parser "Name :" als Sprungmarke gelesen. Damit wurde
; aus "UpdateWorld : RenderWorld" zweimal die Marke "lbl_updateworld" und
; **kein einziger Aufruf** - der Befehl verschwand lautlos. Aufgefallen ist es
; nur, weil g++ ueber die doppelte Marke stolperte; bei einem einzigen
; Vorkommen haette gar nichts gemeldet.
;
; Gemessen am Original (Blitz3D 1.108c): "meinlabel:" in eigener Zeile ergibt
; dort "Function 'meinlabel' not found" - der Bezeichner ist ein Aufruf, der
; Doppelpunkt der Anweisungstrenner. Sprungmarken schreibt Blitz3D
; ausschliesslich als ".name".
;
; Dieser Test kommt ohne Grafik aus, damit die Wirkung an einer Zahl ablesbar
; ist statt an einem Bild.

Global n = 0

; --- 1) zwei parameterlose Aufrufe, durch Doppelpunkte getrennt.
;        Mit dem alten Parser entstanden hier zwei gleichnamige Sprungmarken
;        und kein Aufruf; das Emittat liess sich nicht einmal uebersetzen.
Zaehle : Zaehle
If n = 2 Then Print "zwei aufrufe" Else Print "FEHLER zwei aufrufe: " + n

; --- 2) drei in einer Zeile, gemischt mit einem Befehl mit Argumenten
n = 0
Zaehle : Zaehle : Zaehle
If n = 3 Then Print "drei aufrufe" Else Print "FEHLER drei aufrufe: " + n

; --- 3) die Klammerform daneben
n = 0
Zaehle() : Zaehle()
If n = 2 Then Print "mit klammern" Else Print "FEHLER mit klammern: " + n

; --- 4) ein Aufruf am Zeilenende, gefolgt von einem Doppelpunkt
n = 0
Zaehle :
If n = 1 Then Print "trenner am ende" Else Print "FEHLER trenner am ende: " + n

; --- 5) der Trenner trennt auch Zuweisungen von Aufrufen
n = 0
x = 5 : Zaehle : y = x + 1
If n = 1 And y = 6 Then Print "gemischt" Else Print "FEHLER gemischt"

; --- 6) Sprungmarken gibt es weiterhin, aber nur mit fuehrendem Punkt
n = 0
Goto weiter
Zaehle
.weiter
If n = 0 Then Print "sprungmarke" Else Print "FEHLER sprungmarke"

Print "fertig"
End

Function Zaehle()
  n = n + 1
End Function
