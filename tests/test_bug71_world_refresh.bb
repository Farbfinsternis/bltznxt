; BUG-71 - ein Befehl, der die Weltlage liest, muss die frische Lage sehen.
;
; Im Original rechnet `getWorldTform()` verzoegert nach, sobald jemand liest
; (`blitz3d/entity.cpp`, `invalidateWorld`). Ein `PositionEntity` wirkt damit
; sofort - fuer jeden lesenden Befehl **und** fuer das Bild.
;
; Bei uns schrieb lange nur `UpdateWorld` die Weltmatrix. Jeder Leser rechnete
; deshalb mit der Lage der vorigen Runde: eine Kamera, die in derselben
; Schleifenrunde versetzt und dann ausgerichtet wird - also eine ganz
; gewoehnliche Kamerafahrt -, zeigte ins Leere.
;
; Dieser Test setzt und liest **ohne UpdateWorld dazwischen**. Genau darin
; liegt seine Aussage; ein UpdateWorld an der falschen Stelle wuerde ihn
; wertlos machen.
;
; Alle Werte am laufenden Original gemessen (2026-09-11).

Graphics3D 640, 480, 16, 2
SetBuffer BackBuffer()

; --- 1) EntityX/Y/Z mit glob=1 ---
a = CreatePivot()
PositionEntity a, 3, 4, 5
If EntityX(a,1) = 3 And EntityY(a,1) = 4 And EntityZ(a,1) = 5 Then Print "entityxyz global" Else Print "FEHLER entityxyz global"

; Auch ein zweites Mal in derselben Runde - die Auffrischung darf nichts
; einfrieren.
PositionEntity a, -7, 0, 0
If EntityX(a,1) = -7 Then Print "entityx zweimal" Else Print "FEHLER entityx zweimal"

; --- 2) Durch eine Kette hindurch ---
;
; Das Kind muss die frische Lage des ELTERNTEILS sehen, nicht nur die eigene.
vater = CreatePivot()
kind = CreatePivot(vater)
PositionEntity kind, 0, 0, 1
PositionEntity vater, 10, 0, 0
If EntityX(kind,1) = 10 And EntityZ(kind,1) = 1 Then Print "kette" Else Print "FEHLER kette"

; Und eine Ebene tiefer, mit einer Drehung dazwischen.
enkel = CreatePivot(kind)
PositionEntity enkel, 0, 0, 1
RotateEntity vater, 0, 90, 0
If Abs(EntityX(enkel,1) - 8) < 0.001 Then Print "kette mit drehung" Else Print "FEHLER kette mit drehung"

; --- 3) Die Winkel-Getter mit glob=1 ---
b = CreatePivot()
RotateEntity b, 0, 45, 0
If Abs(EntityYaw(b,1) - 45) < 0.01 Then Print "entityyaw global" Else Print "FEHLER entityyaw global"

; --- 4) EntityDistance ---
d1 = CreatePivot()
d2 = CreatePivot()
PositionEntity d1, 0, 0, 0
PositionEntity d2, 0, 0, 10
If Abs(EntityDistance(d1,d2) - 10) < 0.001 Then Print "entitydistance" Else Print "FEHLER entitydistance"

; --- 5) PointEntity - der Fall, an dem BUG-71 aufgefallen ist ---
;
; Versetzen und in derselben Runde ausrichten. Das Ziel liegt bei (0,0,0),
; die Kamera bei (5,0,0) - sie muss also nach -x schauen. Gemessen ist das
; Gier **+90**: `Vector::yaw()` ist im Original -atan2(x,z), und eine Entity
; mit Gier 90 blickt entlang (-sin,0,cos) = (-1,0,0).
ziel = CreatePivot()
auge = CreatePivot()
PositionEntity auge, 5, 0, 0
PointEntity auge, ziel
If Abs(EntityYaw(auge) - 90) < 0.01 Then Print "pointentity" Else Print "FEHLER pointentity"

; --- 6) PositionEntity mit glob=1 auf ein Kind ---
;
; Die Weltvorgabe wird ueber die Weltmatrix des Elternteils lokal gemacht -
; also muss auch die frisch sein.
p2 = CreatePivot()
k2 = CreatePivot(p2)
PositionEntity p2, 100, 0, 0
PositionEntity k2, 101, 0, 0, 1
If Abs(EntityX(k2) - 1) < 0.001 Then Print "position global auf kind" Else Print "FEHLER position global auf kind"

; --- 7) TranslateEntity mit glob=1 ---
p3 = CreatePivot()
k3 = CreatePivot(p3)
PositionEntity p3, 0, 0, 0
RotateEntity p3, 0, 90, 0
TranslateEntity k3, 0, 0, 1, 1
If Abs(EntityZ(k3,1) - 1) < 0.001 Then Print "translate global" Else Print "FEHLER translate global"

; --- 8) EntityParent mit glob=1 haelt die Weltlage ---
p4 = CreatePivot()
PositionEntity p4, 20, 0, 0
frei = CreatePivot()
PositionEntity frei, 25, 0, 0
EntityParent frei, p4, 1
If Abs(EntityX(frei,1) - 25) < 0.001 And Abs(EntityX(frei) - 5) < 0.001 Then Print "entityparent global" Else Print "FEHLER entityparent global"

; --- 9) MeshesIntersect sieht die frische Lage ---
m1 = CreateCube()
m2 = CreateCube()
PositionEntity m2, 50, 0, 0
If MeshesIntersect(m1,m2) = 0 Then Print "meshes weit auseinander" Else Print "FEHLER meshes weit auseinander"
PositionEntity m2, 0, 0, 0
If MeshesIntersect(m1,m2) <> 0 Then Print "meshes aufeinander" Else Print "FEHLER meshes aufeinander"

Print "fertig"
