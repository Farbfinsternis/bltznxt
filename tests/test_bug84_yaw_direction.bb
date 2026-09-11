; BUG-84 - eine Drehung um die Y-Achse lief bei uns herum.
;
; Referenz: `blitz3d/geom.h:450` schreibt die drei Achsenmatrizen als ihre
; Spalten, und `rotationMatrix` ist `yawMatrix*pitchMatrix*rollMatrix`:
;
;   pitchMatrix(q)  (1,0,0)      (0,cos,sin)   (0,-sin,cos)
;   yawMatrix(q)    (cos,0,sin)  (0,1,0)       (-sin,0,cos)
;   rollMatrix(q)   (cos,sin,0)  (-sin,cos,0)  (0,0,1)
;
; Bei uns stand das Sinus-Vorzeichen der **Gierdrehung** umgekehrt. Pitch und
; Roll stimmten, nur Yaw lief herum.
;
; Warum das so lange getragen hat: unsere Sichtmatrix spiegelt die z-Achse
; (Entities blicken nach +Z, GL nach -Z), und ein gespiegeltes Gier unter
; einer gespiegelten Sicht ergibt ein Bild, das *plausibel* aussieht. Kein
; einziger der 105 Tests hat die Drehrichtung je gegen das Original gehalten -
; deshalb dieser hier.
;
; Alle Werte am laufenden Original gemessen (2026-09-11).

Graphics3D 640, 480, 16, 2
SetBuffer BackBuffer()

; --- 1) Ein Kind unter einem gedrehten Elternteil ---
;
; Der Kern des Fehlers in einer Zeile: Kind bei (0,0,1), Elternteil um 90
; Grad gegiert. Das Original setzt das Kind auf x = -1, wir setzten es auf
; +1 - ohne jede Meldung, in jedem Programm mit einer gedrehten Hierarchie.
p = CreatePivot()
k = CreatePivot(p)
PositionEntity k, 0, 0, 1
RotateEntity p, 0, 90, 0
UpdateWorld
If EntityX(k, 1) < -0.99 And EntityX(k, 1) > -1.01 Then Print "gier 90 nach -x" Else Print "FEHLER gier 90 nach -x"
If Abs(EntityZ(k, 1)) < 0.01 Then Print "gier 90 z ist null" Else Print "FEHLER gier 90 z ist null"

RotateEntity p, 0, -90, 0
UpdateWorld
If EntityX(k, 1) > 0.99 Then Print "gier -90 nach +x" Else Print "FEHLER gier -90 nach +x"

; Eine halbe Drehung ist vorzeichenblind und war auch vorher richtig - sie
; steht hier als Gegenprobe, dass nicht einfach alles gespiegelt wurde.
RotateEntity p, 0, 180, 0
UpdateWorld
If EntityZ(k, 1) < -0.99 Then Print "gier 180 nach -z" Else Print "FEHLER gier 180 nach -z"

; --- 2) Pitch und Roll waren nie betroffen ---
;
; Wichtig als Gegenprobe: der Fix darf genau eine Achse bewegen.
RotateEntity p, 90, 0, 0
UpdateWorld
If EntityY(k, 1) < -0.99 Then Print "nick 90 nach -y" Else Print "FEHLER nick 90 nach -y"

q = CreatePivot()
r = CreatePivot(q)
PositionEntity r, 1, 0, 0
RotateEntity q, 0, 0, 90
UpdateWorld
If EntityY(r, 1) > 0.99 Then Print "roll 90 nach +y" Else Print "FEHLER roll 90 nach +y"

; --- 3) Dieselbe Drehung an der Geometrie ---
;
; RotateMesh geht durch dieselbe Matrix. Gemessen: der erste Vertex des
; Wuerfels liegt bei (-1,1,-1) und nach einer Gierdrehung um 90 Grad bei
; x = +1. Verglichen wird mit Spielraum, weil unsere Drehung eine Hundertstel
; Genauigkeit verliert, wo das Original exakt bleibt (A-03).
c = CreateCube()
s = GetSurface(c, 1)
If VertexX#(s, 0) < -0.99 And VertexZ#(s, 0) < -0.99 Then Print "vertex start" Else Print "FEHLER vertex start"

RotateMesh c, 0, 90, 0
If VertexX#(s, 0) > 0.99 Then Print "vertex nach gier" Else Print "FEHLER vertex nach gier"
If VertexZ#(s, 0) < -0.99 Then Print "vertex z nach gier" Else Print "FEHLER vertex z nach gier"

; --- 4) TurnEntity dreht relativ und geht durch dieselbe Matrix ---
t = CreatePivot()
u = CreatePivot(t)
PositionEntity u, 0, 0, 1
TurnEntity t, 0, 45, 0
TurnEntity t, 0, 45, 0
UpdateWorld
If EntityX(u, 1) < -0.99 Then Print "turn zweimal 45" Else Print "FEHLER turn zweimal 45"

; --- 5) Der gelesene Winkel passt weiter zum gesetzten ---
;
; Die Rueckrechnung aus der Weltmatrix musste mit dem Vorzeichen mitziehen;
; ohne das haetten die Getter aus BUG-83 ploetzlich das Negative gemeldet.
v = CreatePivot()
RotateEntity v, 0, 30, 0
UpdateWorld
If EntityYaw(v) > 29.9 And EntityYaw(v) < 30.1 Then Print "gier gelesen" Else Print "FEHLER gier gelesen"
RotateEntity v, 20, -70, 40
UpdateWorld
If EntityYaw(v) < -69.9 And EntityYaw(v) > -70.1 Then Print "gier negativ gelesen" Else Print "FEHLER gier negativ gelesen"

Print "fertig"
