; BUG-50 - Blitz3D kennt die drei Vergleichsoperatoren in beiden Reihenfolgen.
;
; Am Original gemessen (Blitz3D 11.8), jeweils in beide Richtungen belegt, damit
; nicht nur die Annahme, sondern auch die Bedeutung feststeht:
;
;   "=>" wie ">="   5 => 3 -> 1,  3 => 5 -> 0
;   "=<" wie "<="   5 =< 3 -> 0,  3 =< 5 -> 1
;   "><" wie "<>"   5 >< 3 -> 1,  3 >< 3 -> 0
;
; Ein Zwischenraum ist dabei NICHT erlaubt - "5 = > 3" lehnt das Original ab.
; Das ergibt sich hier von selbst, weil der Lexer nur unmittelbar benachbarte
; Zeichen zusammenfasst; siehe neg_bug50_spaced_operator.bb.
;
; Die Aliase werden auf die kanonische Schreibweise normalisiert, damit Parser
; und Emitter nur eine Form kennen muessen.

; --- 1) die drei Aliase, beide Richtungen
Print 5 => 3
Print 3 => 5
Print 5 =< 3
Print 3 =< 5
Print 5 >< 3
Print 3 >< 3

; --- 2) die kanonischen Formen muessen unveraendert bleiben
Print 5 >= 3
Print 5 <= 3
Print 5 <> 3
Print 5 < 3
Print 5 > 3

; --- 3) und die Zuweisung darf das "=" nicht verlieren
x = 5
Print x
y = -3
Print y
If x = 5 Then Print "gleich"

; --- 4) in der Praxis kommt es so vor
zeit = 1200
If zeit => 1000 Then Print "abgelaufen"
If zeit =< 2000 Then Print "im Rahmen"
If zeit >< 0 Then Print "gesetzt"
