; BUG-134/BUG-220 - UpdateNormals mittelt ueber Ecken an derselben Position.
;
; Das Original sammelt je Surface die normierten Dreiecksnormalen nach der
; Position (Surface::updateNormals); ein Wuerfel bekommt gerundete Ecken.
; BLTZNXT mittelte je Vertex-Index, Sphere-Maps auf Wuerfeln sahen anders aus
; (samples/mak/multi_tex). Zwei Surfaces werden nicht zusammengelegt.
; Erwartung: Ausgabe von Blitz3D 11.8.
Graphics3D 320,240,0,2
c=CreateCube() : UpdateNormals c
s=GetSurface(c,1)
For v=0 To 7
	Print "wuerfel "+v+": "+N(s,v)
Next

; eigenes Netz: zwei Dreiecke verschiedener Groesse teilen eine Position,
; aber keine Vertex-Nummer; ein drittes ist entartet
m=CreateMesh()
a=CreateSurface(m)
AddVertex a,0,0,0 : AddVertex a,1,0,0 : AddVertex a,0,1,0
AddVertex a,0,0,0 : AddVertex a,0,0,4 : AddVertex a,4,0,0
AddVertex a,5,5,5 : AddVertex a,5,5,5 : AddVertex a,5,5,5
AddTriangle a,0,1,2 : AddTriangle a,3,4,5 : AddTriangle a,6,7,8
b=CreateSurface(m)
AddVertex b,0,0,0 : AddVertex b,0,1,0 : AddVertex b,0,0,1
AddTriangle b,0,1,2
UpdateNormals m
For v=0 To 5
	Print "netz a "+v+": "+N(a,v)
Next
Print "netz b 0: "+N(b,0)
Print "DONE"
End

Function N$(s,v)
	Return Int(VertexNX(s,v)*10000)+" "+Int(VertexNY(s,v)*10000)+" "+Int(VertexNZ(s,v)*10000)
End Function
