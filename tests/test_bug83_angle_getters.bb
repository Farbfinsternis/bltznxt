; BUG-83: EntityPitch/Yaw/Roll lesen die LAGE zurueck, nicht das, was
; RotateEntity hineingeschrieben hat.
;
; Referenz: am laufenden Original gemessen (2026-09-10). Der Bereich ist
; (-180, 180], und jenseits von 90 Grad Nick kippt die Zerlegung - "pitch 200"
; ist dieselbe Lage wie "pitch -20, yaw -180, roll 180". Vorher gaben wir
; zurueck, was gesetzt wurde: 200, 370, 181.2.
;
; Zur Genauigkeit: von den 27 Zahlen unten sind 21 Zeichen fuer Zeichen die
; des Originals. Die sechs uebrigen weichen um 1 in der letzten Stelle ab,
; weil unser Int() abschneidet und Blitz3D rundet (A-03). Das ist gemessen und
; nicht vermutet: mit *10000 statt *100 bleibt die Abweichung bei 1, sie
; waechst also nicht mit - die Winkel stimmen auf ein Zehntausendstel Grad.

Graphics3D 640,480,0,2

Global e = CreateCube()

Function Zeig(was$)
  Print was + " -> p=" + Int(EntityPitch(e)*100) + " y=" + Int(EntityYaw(e)*100) + " r=" + Int(EntityRoll(e)*100)
End Function

; --- ueber den Bereich hinaus gesetzt
RotateEntity e, 0, 370.0, 0    : Zeig("yaw 370   ")
RotateEntity e, 0, 181.2, 0    : Zeig("yaw 181.2 ")
RotateEntity e, 0, -180.0, 0   : Zeig("yaw -180  ")

; --- jenseits von 90 Grad Nick kippt die Zerlegung, sie schlaegt nicht
;     nur den Wert um: aus 200 wird -20, und Gier und Rollen springen um 180
RotateEntity e, 200.0, 0, 0    : Zeig("pitch 200 ")
RotateEntity e, 100.0, 30.0, 0 : Zeig("p100 y30  ")

; --- Rollen bleibt ohne Kippen, nur der Bereich wirkt
RotateEntity e, 0, 0, 200.0    : Zeig("roll 200  ")

; --- im Bereich aendert sich nichts
RotateEntity e, 45.0, 45.0, 45.0 : Zeig("45/45/45  ")

; --- der Fall, in dem es in einem echten Programm zubeisst: ein Winkel
;     wird zurueckgelesen und weitergerechnet
RotateEntity e, 0, 170.0, 0
TurnEntity e, 0, 20.0, 0
Zeig("170 + 20  ")
RotateEntity e, 0, EntityYaw(e) + 20.0, 0
Zeig("davon +20 ")
