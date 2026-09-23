; CopyEntity - eine neue Instanz *derselben* Geometrie.
;
; Referenz: bbCopyEntity ruft e->getObject()->copy() (bbruntime/bbblitz3d.cpp),
; und Object::copy() (blitz3d/object.cpp) klont die Entity, kopiert danach
; rekursiv die Kinder an die Kopie und haengt sie zuletzt an den Parent.
; MeshModel::MeshModel(const MeshModel&) uebernimmt dabei den refgezaehlten
; `rep` (blitz3d/meshmodel.cpp:167) - Geometrie wird geteilt, nicht kopiert.
; Genau darin unterscheidet sich CopyEntity von CopyMesh, das eine tiefe
; Kopie anlegt.
;
; Der Fang, den man nicht ableiten kann: **Camera, Light und Terrain
; ueberschreiben `clone()` nicht.** Sie erben Object::clone(), das ein nacktes
; Object baut. Die Kopie einer Kamera ist deshalb ein Pivot, keine Kamera.
;
; Alle Werte unten sind am laufenden Original gemessen (2026-09-11), nicht
; aus dem Quelltext abgeleitet.

Graphics3D 640, 480, 16, 2
SetBuffer BackBuffer()

cam = CreateCamera()
lit = CreateLight()
piv = CreatePivot()

; --- 1) Welche Klasse hat die Kopie? ---
;
; Gemessen: Mesh -> Mesh, Light -> Pivot, Camera -> Pivot, Pivot -> Pivot.
; Ein Licht laesst sich also nicht vervielfaeltigen; man bekommt stillschweigend
; einen leeren Knoten. Das ist keine Nachlaessigkeit hier, sondern das
; gemessene Verhalten des Originals.
cube = CreateCube()
If EntityClass$(CopyEntity(cube)) = "Mesh"  Then Print "mesh bleibt mesh"  Else Print "FEHLER mesh bleibt mesh"
If EntityClass$(CopyEntity(lit))  = "Pivot" Then Print "licht wird pivot"  Else Print "FEHLER licht wird pivot"
If EntityClass$(CopyEntity(cam))  = "Pivot" Then Print "kamera wird pivot" Else Print "FEHLER kamera wird pivot"
If EntityClass$(CopyEntity(piv))  = "Pivot" Then Print "pivot bleibt pivot" Else Print "FEHLER pivot bleibt pivot"

; --- 2) Was die Kopie vom Original erbt ---
;
; Name, lokale Lage und Winkel wandern mit; die Kinder werden rekursiv
; mitkopiert, samt Enkeln. Ohne Parent ist die Kopie eine Wurzel.
NameEntity     cube, "CUBE"
PositionEntity cube, 1, 2, 3
kid = CreateSphere(8, cube)
NameEntity     kid, "KID"
enkel = CreatePivot(kid)
NameEntity     enkel, "ENKEL"

c = CopyEntity(cube)
If EntityName$(c) = "CUBE" Then Print "name" Else Print "FEHLER name"
If EntityX(c) = 1 And EntityY(c) = 2 And EntityZ(c) = 3 Then Print "lage" Else Print "FEHLER lage"
If CountChildren(c) = 1 Then Print "ein kind" Else Print "FEHLER ein kind"
If EntityName$(GetChild(c, 1)) = "KID" Then Print "kindname" Else Print "FEHLER kindname"
If CountChildren(GetChild(c, 1)) = 1 Then Print "enkel da" Else Print "FEHLER enkel da"
If EntityName$(GetChild(GetChild(c, 1), 1)) = "ENKEL" Then Print "enkelname" Else Print "FEHLER enkelname"
If GetParent(c) = 0 Then Print "wurzel" Else Print "FEHLER wurzel"
If CountSurfaces(c) = 1 Then Print "eine flaeche" Else Print "FEHLER eine flaeche"

; --- 3) Die Geometrie ist geteilt, und zwar in beide Richtungen ---
;
; Gemessen: der erste Vertex liegt vorher in beiden bei x=-1. Nach einem
; RotateMesh auf das *Original* bewegt sich auch der Vertex der *Kopie*, und
; ein ScaleMesh auf die *Kopie* bewegt umgekehrt den des *Originals*.
; Verglichen wird hier nur "bewegen sich beide gleich" - der Absolutwert
; nach einer Y-Drehung weicht vom Original ab, das ist nicht CopyEntity,
; siehe BUG-84.
s1 = GetSurface(cube, 1)
s2 = GetSurface(c, 1)
If VertexX#(s1, 0) = VertexX#(s2, 0) Then Print "gleicher start" Else Print "FEHLER gleicher start"

vorher# = VertexX#(s1, 0)
RotateMesh cube, 0, 90, 0
If VertexX#(s1, 0) = VertexX#(s2, 0) And VertexX#(s1, 0) <> vorher# Then Print "original zieht kopie mit" Else Print "FEHLER original zieht kopie mit"

vorher2# = VertexX#(s2, 0)
ScaleMesh c, 2, 2, 2
If VertexX#(s1, 0) = VertexX#(s2, 0) And VertexX#(s2, 0) <> vorher2# Then Print "kopie zieht original mit" Else Print "FEHLER kopie zieht original mit"

; --- 4) Mit Parent bleibt die *lokale* Lage stehen ---
;
; Die Doku sagt "created at the parent entity's position" - gemessen gilt das
; nur, wenn das Original lokal auf 0,0,0 sitzt. Sonst kommt die kopierte
; lokale Lage zur Lage des Parents hinzu: lokal 1, Welt 11.
box = CreateCube()
PositionEntity box, 10, 0, 0
c2 = CopyEntity(cube, box)
UpdateWorld
If EntityX(c2) = 1 And EntityX(c2, 1) = 11 Then Print "lokal bleibt lokal" Else Print "FEHLER lokal bleibt lokal"

flat = CreateCube()
c3 = CopyEntity(flat, box)
UpdateWorld
If EntityX(c3) = 0 And EntityX(c3, 1) = 10 Then Print "nullpunkt beim parent" Else Print "FEHLER nullpunkt beim parent"

; --- 5) Die Kopie einer Kopie ist wieder eine vollstaendige Kopie ---
c4 = CopyEntity(c)
If EntityClass$(c4) = "Mesh" And CountChildren(c4) = 1 Then Print "kopie der kopie" Else Print "FEHLER kopie der kopie"

; --- 6) entfallen: CopyEntity(0) war hier bis 2026-09-23 als folgenlos
; festgehalten - nie am Original gemessen. Seit BUG-170 beendet es das
; Programm mit "Entity does not exist" (Tests test_bug170_*).

; --- 7) FreeEntity auf die Kopie laesst das Original stehen ---
;
; Das ist die Probe darauf, dass der geteilte Flaechenspeicher refgezaehlt
; ist: wird er beim Freigeben der Kopie mit abgeraeumt, verliert das Original
; seine Geometrie.
FreeEntity c
If CountSurfaces(cube) = 1 And VertexX#(s1, 0) <> 0 Then Print "kopie weg, original steht" Else Print "FEHLER kopie weg, original steht"

Print "fertig"
