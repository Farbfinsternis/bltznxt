; BUG-87 - die Gegenprobe: welche Sprungziele gueltig sind.
;
; Das Original meldet `Undefined label 'x'` fuer `Goto`, `Gosub` und
; `Restore` auf ein Label, das es im selben Bereich nicht gibt; uns fehlte
; diese Diagnose ganz, und der Nutzer bekam statt ihrer eine g++-Meldung
; ueber erzeugten Code.
;
; Dieser Test sichert die andere Haelfte: was gueltig ist, muss gueltig
; bleiben. Die drei Regeln dahinter sind am Original gemessen (2026-09-11):
;
;   1. Ein Label darf **hinter** seiner Verwendung stehen.
;   2. Ein Label in einem Block (If, While, ...) zaehlt.
;   3. Labels sind **funktionslokal** - dieselbe Funktion, nicht dasselbe
;      Programm. Die Gegenrichtung steht in neg_bug87_label_fremde_funktion.
;
; Die Ablehnungsfaelle liegen als neg_bug87_* daneben.

; --- 1) Vorwaertssprung: das Ziel kommt erst spaeter ---
Goto weiter
Print "FEHLER uebersprungen"
.weiter
Print "vorwaertssprung"

; --- 2) Gosub und Return ---
Gosub unterprogramm
Print "nach gosub"

; --- 3) Restore auf ein Label, danach Read ---
Restore zweiterblock
Read a
If a = 20 Then Print "restore mit label" Else Print "FEHLER restore mit label"

; Restore ohne Label setzt auf den Anfang zurueck - es braucht keines.
Restore
Read b
If b = 10 Then Print "restore ohne label" Else Print "FEHLER restore ohne label"

; --- 4) Ein Label in einem Block zaehlt ---
If 1 Then Goto imblock
Print "FEHLER block uebersprungen"
.imblock
Print "label im block"

; --- 5) In einer Funktion, mit eigenem Label gleichen Namens ---
;
; Der Name `weiter` ist oben im Hauptprogramm schon vergeben. In der Funktion
; ist es trotzdem ein eigenes Label, weil Labels funktionslokal sind.
If Springe() = 1 Then Print "label in funktion" Else Print "FEHLER label in funktion"

Print "fertig"
End

.unterprogramm
Print "im unterprogramm"
Return

Function Springe()
	Goto weiter
	Return 0
	.weiter
	Return 1
End Function

Data 10,20
.zweiterblock
Data 20
