; BUG-126 - welche Seite eines Dreiecks vorn ist.
;
; Im Original ist die Seite vorn, auf die (b-a)x(c-a) zeigt, im
; linkshaendigen Blitz-Koordinatensystem gerechnet: ein Dreieck, das von vorn
; gesehen im Uhrzeigersinn laeuft. Das gilt fuer jedes Netz gleich - aus
; AddTriangle, aus einer .x- oder .3ds-Datei, aus einem Create-Befehl. Bis
; 2026-09-17 war es bei uns andersherum: selbst gebaute Netze fehlten,
; geladene zeigten ihre Innenseiten und bekamen aus UpdateNormals nach innen
; zeigende Normalen, und die Primitive waren zum Ausgleich umgedreht.
;
; Alle Werte hier sind am Original gemessen (2026-09-17). Die Pixel sind
; lichtunabhaengig (EntityFX 1), bis auf den letzten Abschnitt, der nur
; "beleuchtet / unbeleuchtet" prueft - die Schattierung selbst ist nach der
; Richtlinie frei, die Seite, die das Licht trifft, nicht.
;
; Die Dateien in tests/assets/ erzeugen scripts/make_x_asset.py und
; scripts/make_3ds_asset.py. Die Ecken liegen in Blitz-Koordinaten bei
; (-1,1,0), (1,1,0), (1,-1,0); "cw" laeuft von der Kamera aus im
; Uhrzeigersinn.

Graphics3D 320, 240, 0, 2
Global cam = CreateCamera()
PositionEntity cam, 0, 0, -4
CameraClsColor cam, 0, 0, 0

Function Pixel(x, y)
	RenderWorld
	Return ReadPixel(x, y) And $FFFFFF
End Function

Function Weiss(m, fx)
	EntityFX m, fx
	EntityColor m, 255, 255, 255
End Function

Function Dreieck(i0, i1, i2)
	m = CreateMesh()
	s = CreateSurface(m)
	AddVertex s, -1, 1, 0
	AddVertex s, 1, 1, 0
	AddVertex s, 1, -1, 0
	AddTriangle s, i0, i1, i2
	Return m
End Function

; --- 1) AddTriangle: nur der Uhrzeigersinn ist von vorn zu sehen
m = Dreieck(0, 1, 2) : Weiss m, 1
Print "addtriangle cw " + Pixel(180, 110)
HideEntity m
m = Dreieck(0, 2, 1) : Weiss m, 1
Print "addtriangle ccw " + Pixel(180, 110)
; EntityFX 16 zeigt beide Seiten
EntityFX m, 1 + 16
Print "addtriangle ccw fx16 " + Pixel(180, 110)
HideEntity m

; --- 2) UpdateNormals: die Normale zeigt zur Vorderseite
m = Dreieck(0, 1, 2)
UpdateNormals m
s = GetSurface(m, 1)
Print "updatenormals cw " + Int(VertexNX(s, 0)) + " " + Int(VertexNY(s, 0)) + " " + Int(VertexNZ(s, 0))
HideEntity m

; --- 3) .x ohne MeshNormals: dieselbe Regel, Normalen aus UpdateNormals
m = LoadMesh("tests/assets/test_tri_cw.x") : Weiss m, 1
Print "x cw " + Pixel(180, 110) + " nz " + Int(VertexNZ(GetSurface(m, 1), 0))
HideEntity m
m = LoadMesh("tests/assets/test_tri_ccw.x") : Weiss m, 1
Print "x ccw " + Pixel(180, 110) + " nz " + Int(VertexNZ(GetSurface(m, 1), 0))
HideEntity m

; --- 4) .3ds: der Loader tauscht y und z und damit den Umlaufsinn
m = LoadMesh("tests/assets/test_tri_cw.3ds") : Weiss m, 1
Print "3ds cw " + Pixel(180, 110) + " nz<0 " + (VertexNZ(GetSurface(m, 1), 0) < 0)
HideEntity m
m = LoadMesh("tests/assets/test_tri_ccw.3ds") : Weiss m, 1
Print "3ds ccw " + Pixel(180, 110) + " nz<0 " + (VertexNZ(GetSurface(m, 1), 0) < 0)
HideEntity m

; --- 5) Primitive: von aussen sichtbar, von innen nicht
m = CreateCube() : Weiss m, 1
Print "cube aussen " + Pixel(160, 120)
s = GetSurface(m, 1)
Print "cube dreieck 0: " + TriangleVertex(s, 0, 0) + "," + TriangleVertex(s, 0, 1) + "," + TriangleVertex(s, 0, 2)
ScaleEntity m, 10, 10, 10
Print "cube innen " + Pixel(160, 120)
HideEntity m
m = CreateSphere() : Weiss m, 1
Print "sphere aussen " + Pixel(160, 120)
ScaleEntity m, 10, 10, 10
Print "sphere innen " + Pixel(160, 120)
HideEntity m
m = CreateCylinder() : Weiss m, 1
Print "cylinder aussen " + Pixel(160, 120)
ScaleEntity m, 10, 10, 10
Print "cylinder innen " + Pixel(160, 120)
HideEntity m
m = CreateCone() : Weiss m, 1
Print "cone aussen " + Pixel(160, 100)
ScaleEntity m, 10, 10, 10
Print "cone innen " + Pixel(160, 120)
HideEntity m

; --- 6) Licht trifft die Vorderseite eines geladenen Dreiecks
AmbientLight 0, 0, 0
light = CreateLight(1)
m = LoadMesh("tests/assets/test_tri_cw.x")
EntityColor m, 255, 255, 255
Print "licht von vorn " + (Pixel(180, 110) > 0)
RotateEntity light, 0, 180, 0
Print "licht von hinten " + (Pixel(180, 110) > 0)

Print "fertig"
End
