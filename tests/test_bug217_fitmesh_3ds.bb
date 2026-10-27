; BUG-217: zwei Ursachen, warum samples/Richard_Betson/emerald_gate anders aussah.
;
; 1) FitMesh setzt das Netz mit seiner Mitte in die Mitte der Zielbox
;    (bbFitMesh: t.v = box.centre() - t.m * curr_box.centre()). Mit uniform
;    gilt der x-Faktor nur, wenn er echt kleiner ist als y und z, der
;    y-Faktor ebenso, sonst der z-Faktor - auch wenn der groesser ist.
;    Bei uns lag das Netz an der Mindestecke; das Tor stand neben dem Wasser.
; 2) Der .3ds-Lader des Originals liest von einem Material nur Farbe und
;    Textur; Glanz und Transparenz uebergeht er, und die Vertices bleiben je
;    Objekt so geteilt wie in der Datei, gleich welche Glaettungsgruppe.
;    Bei uns machte der Glanz den inneren Ring des Tors weiss.
;
; Die Testdatei tests/assets/bug217_material.3ds ist ein Wuerfel mit Farbe
; 200,100,50, 50 % Glanz, 50 % Transparenz und einer Glaettungsgruppe je
; Dreieck (erzeugt mit build/bug217_20261027/mk3ds.py). Alle Zeilen sind am
; Original gemessen (Blitz3D 1.108, build/bug217_20261027/fit und test).
Graphics3D 320,240,0,2
Function Box$(m)
	x0#=1000000.0:y0#=1000000.0:z0#=1000000.0:x1#=-1000000.0:y1#=-1000000.0:z1#=-1000000.0
	For s=1 To CountSurfaces(m)
		sf=GetSurface(m,s)
		For i=0 To CountVertices(sf)-1
			x#=VertexX(sf,i):y#=VertexY(sf,i):z#=VertexZ(sf,i)
			If x<x0 Then x0=x
			If y<y0 Then y0=y
			If z<z0 Then z0=z
			If x>x1 Then x1=x
			If y>y1 Then y1=y
			If z>z1 Then z1=z
		Next
	Next
	Return x0+","+y0+","+z0+" .. "+x1+","+y1+","+z1
End Function
m=CreateCube() : FitMesh m,0,0,0,4,2,6 : Print "fit "+Box(m)
m=CreateCube() : FitMesh m,0,0,0,4,2,6,1 : Print "fit uniform "+Box(m)
m=CreateCube() : FitMesh m,0,0,0,4,4,6,1 : Print "fit gleich xy "+Box(m)
m=CreateCube() : FitMesh m,0,0,0,6,4,4,1 : Print "fit gleich yz "+Box(m)
m=CreateCube() : FitMesh m,0,0,0,4,6,4,1 : Print "fit gleich xz "+Box(m)
m=CreateCube() : PositionMesh m,5,1,0 : ScaleMesh m,1,2,0.5 : FitMesh m,-1,-1,-1,2,2,2,1 : Print "fit verschoben "+Box(m)
m=CreateCube() : FitMesh m,1,2,3,-4,2,6 : Print "fit negativ "+Box(m)
m=LoadMesh("tests/assets/bug217_material.3ds")
Print "3ds flaechen "+CountSurfaces(m)+" vertices "+CountVertices(GetSurface(m,1))+" dreiecke "+CountTriangles(GetSurface(m,1))
cam=CreateCamera() : PositionEntity cam,0,0,-5 : CameraClsColor cam,0,0,255
EntityFX m,17
RenderWorld
c=ReadPixel(160,120,BackBuffer()) And $FFFFFF
Print "3ds farbe "+((c Shr 16) And 255)+","+((c Shr 8) And 255)+","+(c And 255)
End
