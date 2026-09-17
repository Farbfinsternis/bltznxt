; BUG-69, BUG-93, BUG-133 - die Vertextabellen von CreateSphere, CreateCylinder
; und CreateCone.
;
; Jede Zeile der .expected ist am Original gemessen (2026-09-17): Positionen,
; Normalen und Texturkoordinaten jedes Vertex (mal 1000, gerundet, damit die
; Formatierung von Str(float) nicht mitspielt - BUG-68) und die drei Indizes
; jedes Dreiecks. Die Generatoren folgen MeshUtil::createSphere/-Cylinder/-Cone
; aus blitz3d/meshutil.cpp.
;
; Vorher: Kugel 576 Vertices und 288 Dreiecke statt 151 und 224, Kegel und
; Zylinder eine Flaeche statt zwei, geneigte Mantelnormalen beim Kegel. Der
; zweite Parameter von CreateCylinder/CreateCone hiess `open` mit Vorgabe 0 und
; bedeutete das Gegenteil von `solid`: CreateCylinder(5,0) hatte Deckel.

Graphics3D 320,240,0,2

Function R$(v#)
	Return Int(Floor(v * 1000 + 0.5))
End Function

Function Dump(name$, m)
	Print name + " surfaces=" + CountSurfaces(m)
	For k = 1 To CountSurfaces(m)
		s = GetSurface(m, k)
		Print " surface " + k + " v=" + CountVertices(s) + " t=" + CountTriangles(s)
		For i = 0 To CountVertices(s) - 1
			Print "  v" + i + " " + R(VertexX(s,i)) + " " + R(VertexY(s,i)) + " " + R(VertexZ(s,i)) + " n " + R(VertexNX(s,i)) + " " + R(VertexNY(s,i)) + " " + R(VertexNZ(s,i)) + " uv " + R(VertexU(s,i)) + " " + R(VertexV(s,i))
		Next
		For i = 0 To CountTriangles(s) - 1
			Print "  t" + i + " " + TriangleVertex(s,i,0) + " " + TriangleVertex(s,i,1) + " " + TriangleVertex(s,i,2)
		Next
	Next
	FreeEntity m
End Function

Dump "CreateSphere()", CreateSphere()
Dump "CreateSphere(2)", CreateSphere(2)
Dump "CreateSphere(3)", CreateSphere(3)
Dump "CreateCylinder()", CreateCylinder()
Dump "CreateCylinder(3)", CreateCylinder(3)
Dump "CreateCylinder(5,0)", CreateCylinder(5,0)
Dump "CreateCone()", CreateCone()
Dump "CreateCone(3)", CreateCone(3)
Dump "CreateCone(5,0)", CreateCone(5,0)
p = CreatePivot()
c = CreateCone(4,1,p)
Print "parent " + (GetParent(c) = p)
End
