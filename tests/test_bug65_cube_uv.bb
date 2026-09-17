; BUG-65 - die Vertextabelle von CreateCube.
;
; Der alte Eintrag sagte "die Texturkoordinaten sind gespiegelt" und "der
; Test dafuer geht erst, wenn BUG-63 behoben ist". Beides war falsch. Es
; fehlten die Flaechenbefehle aus 3D-15; mit ihnen ist die ganze Tabelle als
; Zahlen vergleichbar, und es waren drei verschiedene Dinge: die Reihenfolge
; der Flaechen, die Reihenfolge der Ecken innerhalb einer Flaeche und - nur
; auf vier der sechs Flaechen - die Zuordnung der Texturkoordinaten.
;
; Die 24 Zeilen hier sind am laufenden Original abgelesen, nicht ausgedacht.
; Jede Flaeche laeuft von der oberen linken Ecke im Uhrzeigersinn:
; (0,0), (1,0), (1,1), (0,1).
;
; Die Dreiecke laufen wie im Original: (0,1,2) und (0,2,3). Bis 2026-09-17
; liefen sie hier andersherum, um eine falsch gesetzte Vorderseite im
; Renderer auszugleichen (BUG-126, dort auch die Sichtbarkeit).

Graphics3D 320, 240, 32, 2
cam = CreateCamera()
PositionEntity cam, 0, 0, -6

w = CreateCube()
Global s = GetSurface(w, 1)

If CountVertices(s) = 24 Then Print "24 vertices" Else Print "FEHLER 24 vertices"
If CountTriangles(s) = 12 Then Print "12 dreiecke" Else Print "FEHLER 12 dreiecke"

pruefe 0, -1, 1, -1, 0, 0, -1, 0, 0
pruefe 1, 1, 1, -1, 0, 0, -1, 1, 0
pruefe 2, 1, -1, -1, 0, 0, -1, 1, 1
pruefe 3, -1, -1, -1, 0, 0, -1, 0, 1
pruefe 4, 1, 1, -1, 1, 0, 0, 0, 0
pruefe 5, 1, 1, 1, 1, 0, 0, 1, 0
pruefe 6, 1, -1, 1, 1, 0, 0, 1, 1
pruefe 7, 1, -1, -1, 1, 0, 0, 0, 1
pruefe 8, 1, 1, 1, 0, 0, 1, 0, 0
pruefe 9, -1, 1, 1, 0, 0, 1, 1, 0
pruefe 10, -1, -1, 1, 0, 0, 1, 1, 1
pruefe 11, 1, -1, 1, 0, 0, 1, 0, 1
pruefe 12, -1, 1, 1, -1, 0, 0, 0, 0
pruefe 13, -1, 1, -1, -1, 0, 0, 1, 0
pruefe 14, -1, -1, -1, -1, 0, 0, 1, 1
pruefe 15, -1, -1, 1, -1, 0, 0, 0, 1
pruefe 16, -1, 1, 1, 0, 1, 0, 0, 0
pruefe 17, 1, 1, 1, 0, 1, 0, 1, 0
pruefe 18, 1, 1, -1, 0, 1, 0, 1, 1
pruefe 19, -1, 1, -1, 0, 1, 0, 0, 1
pruefe 20, -1, -1, -1, 0, -1, 0, 0, 0
pruefe 21, 1, -1, -1, 0, -1, 0, 1, 0
pruefe 22, 1, -1, 1, 0, -1, 0, 1, 1
pruefe 23, -1, -1, 1, 0, -1, 0, 0, 1

; Die Dreiecke laufen wie im Original (BUG-126).
If TriangleVertex(s,0,0) = 0 And TriangleVertex(s,0,1) = 1 And TriangleVertex(s,0,2) = 2 Then Print "dreieck 0" Else Print "FEHLER dreieck 0"
If TriangleVertex(s,1,0) = 0 And TriangleVertex(s,1,1) = 2 And TriangleVertex(s,1,2) = 3 Then Print "dreieck 1" Else Print "FEHLER dreieck 1"

Print "fertig"
End

Function pruefe(i, tx#, ty#, tz#, tnx#, tny#, tnz#, tu#, tv#)
	ok = 1
	If VertexX(s,i) <> tx Or VertexY(s,i) <> ty Or VertexZ(s,i) <> tz Then ok = 0
	If VertexNX(s,i) <> tnx Or VertexNY(s,i) <> tny Or VertexNZ(s,i) <> tnz Then ok = 0
	If VertexU(s,i) <> tu Or VertexV(s,i) <> tv Then ok = 0
	If ok Then Print "v" + Str(i) Else Print "FEHLER v" + Str(i)
End Function
