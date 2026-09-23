; 3D-13 (erster Teil) - die Meshbefehle, die keinen Datei-Loader brauchen:
; ScaleMesh, RotateMesh, PositionMesh, FitMesh, FlipMesh, UpdateNormals,
; LightMesh, AddMesh, CopyMesh, CreateMesh, CountSurfaces, MeshesIntersect,
; PaintMesh.
;
; ScaleMesh steht in 22, FlipMesh in 19, CreateMesh in 17, UpdateNormals in
; 12 und FitMesh in 8 der 130 mitgelieferten Beispielprogramme.
;
; Alle diese Befehle arbeiten auf den **Vertices**, nicht auf der
; Transformation der Entity, und rechnen laut Doku vom Ursprung 0,0,0 aus.
;
; Am laufenden Original nachgemessen, weil die Doku es offenlaesst:
;   ScaleMesh ist kumulativ (2.0 -> 4.0 -> 8.0)
;   FitMesh setzt die Mindestecke auf x,y,z, nicht die Mitte;
;     mit uniform gilt der kleinste der drei Faktoren
;   FlipMesh kehrt Umlaufsinn und Normalen um
;   AddMesh fasst in die vorhandene Flaeche zusammen, die Quelle bleibt
;   LightMesh addiert Farbe * (range / Abstand) * max(N.L,0) auf die
;     Vertexfarben und rechnet mit Bytegenauigkeit weiter
;
; **Was dieser Test zusichert:** die zahlenmaessig ablesbaren Zusagen -
; Ausmasse, Flaechenzahl, Unabhaengigkeit der Kopie, Durchdringung. Das
; Aussehen sichert er nicht zu; dafuer stehen die Messungen, die 48 von 48
; vergleichbaren Faellen gegen das Original gehalten haben (siehe DEVLOG).

Graphics3D 320,240,32,2

cam = CreateCamera()
PositionEntity cam, 0, 0, -10
licht = CreateLight()

; --- 1) ScaleMesh ist kumulativ
m = CreateCube()
If MeshWidth(m) = 2 And MeshHeight(m) = 2 And MeshDepth(m) = 2 Then Print "wuerfel roh" Else Print "FEHLER wuerfel roh"
ScaleMesh m, 2, 2, 2
If MeshWidth(m) = 4 Then Print "scale einmal" Else Print "FEHLER scale einmal"
ScaleMesh m, 2, 2, 2
If MeshWidth(m) = 8 Then Print "scale kumulativ" Else Print "FEHLER scale kumulativ"
ScaleMesh m, 0.25, 0.5, 1
If MeshWidth(m) = 2 And MeshHeight(m) = 4 And MeshDepth(m) = 8 Then Print "scale je achse" Else Print "FEHLER scale je achse"
FreeEntity m

; --- 2) FitMesh mit und ohne uniform
m = CreateCube()
FitMesh m, 0, 0, 0, 4, 2, 6
If MeshWidth(m) = 4 And MeshHeight(m) = 2 And MeshDepth(m) = 6 Then Print "fitmesh" Else Print "FEHLER fitmesh"
FreeEntity m

m = CreateCube()
FitMesh m, 0, 0, 0, 4, 2, 6, 1
If MeshWidth(m) = 2 And MeshHeight(m) = 2 And MeshDepth(m) = 2 Then Print "fitmesh uniform" Else Print "FEHLER fitmesh uniform"
FreeEntity m

; --- 3) RotateMesh dreht die Geometrie: 90 Grad Gieren tauscht x und z
m = CreateCube()
ScaleMesh m, 1, 2, 3
RotateMesh m, 0, 90, 0
; Nach einer Drehung ist der Vergleich mit = unbrauchbar: cos(90) ist im
; Gleitkomma nicht genau 0, und 5.9999998 druckt sich als "6".
If Abs(MeshWidth(m) - 6) < 0.001 And Abs(MeshHeight(m) - 4) < 0.001 And Abs(MeshDepth(m) - 2) < 0.001 Then Print "rotatemesh" Else Print "FEHLER rotatemesh"
FreeEntity m

; --- 4) PositionMesh laesst die Ausmasse in Ruhe und verschiebt die Vertices.
;        Nachweisbar ueber MeshesIntersect gegen einen zweiten Wuerfel.
a = CreateCube()
b = CreateCube()
UpdateWorld
If MeshesIntersect(a, b) = 1 Then Print "durchdringung" Else Print "FEHLER durchdringung"
PositionMesh b, 10, 0, 0
UpdateWorld
If MeshWidth(b) = 2 Then Print "positionmesh ausmasse" Else Print "FEHLER positionmesh ausmasse"
If MeshesIntersect(a, b) = 0 Then Print "getrennt" Else Print "FEHLER getrennt"
PositionMesh b, -9, 0, 0
UpdateWorld
If MeshesIntersect(a, b) = 1 Then Print "wieder zusammen" Else Print "FEHLER wieder zusammen"
FreeEntity a
FreeEntity b

; --- 5) CreateMesh ist leer, AddMesh fasst in die vorhandene Flaeche zusammen
leer = CreateMesh()
If CountSurfaces(leer) = 0 Then Print "createmesh leer" Else Print "FEHLER createmesh leer"
If MeshWidth(leer) = 0 Then Print "createmesh ohne ausmass" Else Print "FEHLER createmesh ohne ausmass"

q = CreateCube()
ScaleMesh q, 5, 5, 5
AddMesh q, leer
If CountSurfaces(leer) = 1 Then Print "addmesh flaeche" Else Print "FEHLER addmesh flaeche"
If MeshWidth(leer) = 10 Then Print "addmesh geometrie" Else Print "FEHLER addmesh geometrie"
If CountSurfaces(q) = 1 Then Print "quelle bleibt" Else Print "FEHLER quelle bleibt"

PositionMesh q, 20, 0, 0
AddMesh q, leer
If CountSurfaces(leer) = 1 Then Print "zweimal dieselbe flaeche" Else Print "FEHLER zweimal dieselbe flaeche"
If MeshWidth(leer) = 30 Then Print "addmesh zweimal" Else Print "FEHLER addmesh zweimal"

; --- 6) CopyMesh ist eine tiefe Kopie: die Quelle bleibt unberuehrt
k = CopyMesh(q)
If CountSurfaces(k) = 1 Then Print "copymesh flaeche" Else Print "FEHLER copymesh flaeche"
If MeshWidth(k) = MeshWidth(q) Then Print "copymesh gleich" Else Print "FEHLER copymesh gleich"
ScaleMesh k, 3, 3, 3
If MeshWidth(k) = 30 And MeshWidth(q) = 10 Then Print "copymesh unabhaengig" Else Print "FEHLER copymesh unabhaengig"
FreeEntity k
FreeEntity q
FreeEntity leer

; --- 7) FlipMesh und UpdateNormals lassen Ausmasse und Dreieckszahl in Ruhe
m = CreateCube()
UpdateWorld
RenderWorld
tris = TrisRendered()
FlipMesh m
UpdateWorld
RenderWorld
If TrisRendered() = tris Then Print "flipmesh dreiecke" Else Print "FEHLER flipmesh dreiecke"
If MeshWidth(m) = 2 Then Print "flipmesh ausmasse" Else Print "FEHLER flipmesh ausmasse"
UpdateNormals m
UpdateWorld
RenderWorld
If TrisRendered() = tris Then Print "updatenormals" Else Print "FEHLER updatenormals"

; --- 8) LightMesh laeuft und laesst die Geometrie unangetastet.
;        Sichtbar wird es erst mit EntityFX 2; das prueft dieser Test nicht.
EntityFX m, 2
LightMesh m, -255, -255, -255
LightMesh m, 255, 128, 0
LightMesh m, 255, 255, 255, 10, 0, 0, -4
If MeshWidth(m) = 2 Then Print "lightmesh" Else Print "FEHLER lightmesh"
FreeEntity m

; --- 9) PaintMesh ohne Brush-System bleibt folgenlos statt abzustuerzen
m = CreateCube()
PaintMesh m, 0
If MeshWidth(m) = 2 Then Print "paintmesh" Else Print "FEHLER paintmesh"
FreeEntity m

; Ungueltige Handles waren hier bis 2026-09-23 als folgenlos festgehalten -
; nie am Original gemessen. Seit BUG-170 beenden sie das Programm mit der
; Meldung des Debug-Modus; das pruefen die Tests test_bug170_*.

Print "fertig"
