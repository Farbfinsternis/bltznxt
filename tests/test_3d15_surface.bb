; 3D-15 (zweiter Teil) - Flaechen und Vertices.
;
; Diese Befehle machen die Datenseite eines Netzes aus der Sprache heraus
; sichtbar. Das ist die erste Stelle im 3D-Teil, an der eine Zusicherung
; ohne Bildpunktvergleich auskommt - jede Zahl hier ist am laufenden
; Original nachgemessen (62 Werte, 58 gleich; die vier Abweichungen stehen
; unten und in der Buglist).
;
; Vier Ergebnisse haette man anders erwartet:
;
;   * AddVertex und AddTriangle liefern NULLBASIERTE Indizes, obwohl
;     GetSurface einsbasiert zaehlt.
;   * Das dritte Texturmass w wird weggeworfen. VertexW gibt bedingungslos 1
;     zurueck, auch nach AddVertex mit w=0.3.
;   * AddVertex setzt BEIDE Koordinatensaetze, VertexTexCoords nur den
;     angegebenen.
;   * VertexColor klemmt und rechnet Alpha ueber ein Byte: aus 0.5 wird
;     0.498039.
;
; Absichtliche Abweichung: GetSurface(mesh,0) liest im Original hinter den
; Vektor und liefert Muell (gemessen -2013262848). Wir liefern 0.

Graphics3D 320, 240, 32, 2
cam = CreateCamera()
PositionEntity cam, 0, 0, -6

m = CreateMesh()
p("countsurfaces leer", CountSurfaces(m) = 0)

s = CreateSurface(m)
p("createsurface handle", s <> 0)
p("countsurfaces nach create", CountSurfaces(m) = 1)
p("getsurface 1 ist s", GetSurface(m,1) = s)
p("getsurface 1 stabil", GetSurface(m,1) = GetSurface(m,1))
p("getsurface 0", GetSurface(m,0) = 0)
p("getsurface hinter dem ende", GetSurface(m,2) = 0)
p("countvertices leer", CountVertices(s) = 0)
p("counttriangles leer", CountTriangles(s) = 0)

; --- Vertices anlegen: nullbasierte Indizes ---
i0 = AddVertex(s, 1,2,3)
i1 = AddVertex(s, 4,5,6, 0.25, 0.5)
i2 = AddVertex(s, 7,8,9, 0.1, 0.2, 0.3)
p("addvertex indizes", i0 = 0 And i1 = 1 And i2 = 2)
p("countvertices", CountVertices(s) = 3)

t0 = AddTriangle(s, i0, i1, i2)
p("addtriangle index", t0 = 0)
p("counttriangles", CountTriangles(s) = 1)
p("trianglevertex", TriangleVertex(s,0,0) = 0 And TriangleVertex(s,0,1) = 1 And TriangleVertex(s,0,2) = 2)

p("vertexkoordinaten", VertexX(s,0) = 1 And VertexY(s,0) = 2 And VertexZ(s,0) = 3)
p("normale vorgabe 0", VertexNX(s,0) = 0 And VertexNY(s,0) = 0 And VertexNZ(s,0) = 0)
p("farbe vorgabe weiss", VertexRed(s,0) = 255 And VertexGreen(s,0) = 255 And VertexBlue(s,0) = 255)
p("alpha vorgabe 1", VertexAlpha(s,0) = 1)

; AddVertex fuellt beide Koordinatensaetze
p("addvertex satz 0", VertexU(s,1,0) = 0.25 And VertexV(s,1,0) = 0.5)
p("addvertex satz 1", VertexU(s,1,1) = 0.25 And VertexV(s,1,1) = 0.5)
; ... und wirft das dritte Mass weg
p("vertexw immer 1", VertexW(s,1,0) = 1 And VertexW(s,2,0) = 1)

VertexCoords s, 0, 10, 20, 30
p("vertexcoords", VertexX(s,0) = 10)
VertexNormal s, 0, 0, 1, 0
p("vertexnormal", VertexNY(s,0) = 1)

; VertexTexCoords setzt nur den angegebenen Satz
VertexTexCoords s, 0, 0.75, 0.875
p("texcoords satz 0 gesetzt", VertexU(s,0,0) = 0.75)
p("texcoords satz 1 unberuehrt", VertexU(s,0,1) = 0)
VertexTexCoords s, 0, 0.1, 0.2, 1, 1
p("texcoords satz 1 gesetzt", Abs(VertexU(s,0,1) - 0.1) < 0.0001)
p("texcoords satz 0 unberuehrt", VertexU(s,0,0) = 0.75)

; VertexColor klemmt, und Alpha laeuft ueber ein Byte
VertexColor s, 0, 300, -5, 128
p("vertexcolor geklemmt", VertexRed(s,0) = 255 And VertexGreen(s,0) = 0 And VertexBlue(s,0) = 128)
p("vertexcolor alpha vorgabe", VertexAlpha(s,0) = 1)
VertexColor s, 0, 10, 20, 30, 0.5
p("vertexcolor alpha ueber byte", Abs(VertexAlpha(s,0) - 0.498039) < 0.00001)
VertexColor s, 0, 10, 20, 30, 2
p("vertexcolor alpha geklemmt", VertexAlpha(s,0) = 1)

; Die Ausmasse folgen den Vertices: x von 4 bis 10
p("meshwidth folgt den vertices", MeshWidth(m) = 6)

; --- zweite Flaeche, FindSurface, GetSurfaceBrush ---
b = CreateBrush()
BrushColor b, 10, 20, 30
s2 = CreateSurface(m, b)
p("countsurfaces zwei", CountSurfaces(m) = 2)
p("getsurface 2 ist s2", GetSurface(m,2) = s2)
p("createsurface behaelt s", GetSurface(m,1) = s)
p("findsurface findet s2", FindSurface(m, b) = s2)
b2 = CreateBrush()
BrushColor b2, 99, 99, 99
p("findsurface ohne treffer", FindSurface(m, b2) = 0)
gb = GetSurfaceBrush(s2)
p("getsurfacebrush kopie", gb <> 0 And gb <> b)

; PaintSurface faerbt nur diese eine Flaeche
PaintSurface s, b2
p("paintsurface findet s", FindSurface(m, b2) = s)

ClearSurface s
p("clearsurface", CountVertices(s) = 0 And CountTriangles(s) = 0)

; --- so baut das Original einen Wuerfel ---
w = CreateCube()
p("wuerfel eine flaeche", CountSurfaces(w) = 1)
ws = GetSurface(w, 1)
p("wuerfel 24 vertices", CountVertices(ws) = 24)
p("wuerfel 12 dreiecke", CountTriangles(ws) = 12)

Print "fertig"
End

Function p(name$, ok)
	If ok Then Print name$ Else Print "FEHLER " + name$
End Function
