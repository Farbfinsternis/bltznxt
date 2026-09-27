; 3D-24 - glTF: LoadAnimMesh (Knotenbaum), LoadMesh (eingeschmolzen),
; Achsen, Umlaufsinn, Accessoren (u8/u16, Streifen, sparse, normierte
; Farben), LoaderMatrix, Bild mit Lightmap auf dem zweiten UV-Satz,
; Alphatest und Blend; Fehlerfaelle.
;
; Die Dateien erzeugt scripts/make_gltf_asset.py. Blitz3D kennt glTF nicht,
; es gibt also nichts am Original zu messen. Die erwarteten Werte sind aus
; der glTF-Geometrie von Hand gerechnet: blitz = (x, y, -z), und aus einem
; Dreieck, das von +Z gegen den Uhrzeigersinn laeuft, wird eines, das von
; -Z im Uhrzeigersinn laeuft (Indizes 0,2,1). Werte x1000 ueber Floor.

Graphics3D 320,240,0,2
SetBuffer BackBuffer()
cam = CreateCamera()
CameraClsColor cam,0,0,0

Function F$(x#)
	Return Int(Floor(x * 1000.0 + 0.5))
End Function
Function W$(e)
	Return F(EntityX(e,True)) + "," + F(EntityY(e,True)) + "," + F(EntityZ(e,True))
End Function
Function L$(e)
	Return F(EntityX(e)) + "," + F(EntityY(e)) + "," + F(EntityZ(e)) + " r " + F(EntityPitch(e)) + "," + F(EntityYaw(e)) + "," + F(EntityRoll(e))
End Function
Function Baum(e, tiefe)
	s$ = ""
	For i = 1 To tiefe : s = s + " " : Next
	v$ = ""
	For i = 1 To CountSurfaces(e)
		v = v + " " + CountVertices(GetSurface(e,i)) + "/" + CountTriangles(GetSurface(e,i))
	Next
	Print s + "[" + EntityName(e) + "] " + EntityClass(e) + " kinder " + CountChildren(e) + v + " | " + L(e) + " | welt " + W(e)
	For i = 1 To CountChildren(e)
		Baum(GetChild(e,i), tiefe + 1)
	Next
End Function
Function Vtx$(s, i)
	Return "v" + i + " " + F(VertexX(s,i)) + "," + F(VertexY(s,i)) + "," + F(VertexZ(s,i)) + " n " + F(VertexNX(s,i)) + "," + F(VertexNY(s,i)) + "," + F(VertexNZ(s,i)) + " uv " + F(VertexU(s,i,0)) + "," + F(VertexV(s,i,0)) + " / " + F(VertexU(s,i,1)) + "," + F(VertexV(s,i,1)) + " rgb " + Int(VertexRed(s,i)) + "," + Int(VertexGreen(s,i)) + "," + Int(VertexBlue(s,i))
End Function
Function Tri$(s, t)
	Return "t" + t + " " + TriangleVertex(s,t,0) + "," + TriangleVertex(s,t,1) + "," + TriangleVertex(s,t,2)
End Function

; --- 1) LoadAnimMesh: der Knotenbaum ---
m = LoadAnimMesh("tests/assets/test_gltf_szene.glb")
Baum(m, 0)
q = FindChild(m, "quad")
s1 = GetSurface(q, 1)
s2 = GetSurface(q, 2)
For i = 0 To CountVertices(s1) - 1 : Print Vtx(s1, i) : Next
For t = 0 To CountTriangles(s1) - 1 : Print Tri(s1, t) : Next
For i = 0 To CountVertices(s2) - 1 : Print Vtx(s2, i) : Next
For t = 0 To CountTriangles(s2) - 1 : Print Tri(s2, t) : Next
Print "animseq " + AnimSeq(m)
FreeEntity m

; --- 2) LoadMesh: ein Netz, die Vertices in Weltlage ---
m = LoadMesh("tests/assets/test_gltf_szene.glb")
Print "loadmesh kinder " + CountChildren(m) + " flaechen " + CountSurfaces(m)
Print Vtx(GetSurface(m,1), 0)
FreeEntity m

; --- 3) LoaderMatrix: ohne Spiegelung stehen die glTF-Werte da ---
LoaderMatrix "glb",1,0,0, 0,1,0, 0,0,1
m = LoadAnimMesh("tests/assets/test_gltf_szene.glb")
q = FindChild(m, "quad")
Print "roh quad welt " + W(q) + " " + Tri(GetSurface(q,1), 0)
FreeEntity m
LoaderMatrix "glb",1,0,0, 0,1,0, 0,0,-1

; --- 4) Bild: Lightmap (UV-Satz 1), Alphatest, Blend ---
; Kamera im Ursprung, Blick nach +z; die Tafeln stehen bei z = 5,
; 32 Pixel je Einheit (Zoom 1, 90 Grad), Mitte 160,120.
Function Farbe$(x, y)
	p = ReadPixelFast(x, y) And $FFFFFF
	r = (p Shr 16) And 255 : g = (p Shr 8) And 255 : b = p And 255
	If r > 200 And g < 60 And b < 60 Then Return "rot"
	If b > 200 And r < 60 And g < 60 Then Return "blau"
	If g > 200 And r < 60 And b < 60 Then Return "gruen"
	If r < 20 And g < 20 And b < 20 Then Return "schwarz"
	If Abs(r - 128) < 20 And Abs(g - 128) < 20 And Abs(b - 128) < 20 Then Return "grau"
	Return r + "," + g + "," + b
End Function
t = LoadAnimMesh("tests/assets/test_gltf_bild.gltf")
Print "bild flaechen " + CountSurfaces(GetChild(t,1))
RenderWorld
LockBuffer BackBuffer()
Print "lightmap " + Farbe(96, 120)
Print "maske links " + Farbe(141, 120) + " rechts " + Farbe(179, 120)
Print "blend " + Farbe(224, 120)
Print "daneben " + Farbe(160, 20)
UnlockBuffer BackBuffer()
FreeEntity t

; --- 5) Fehler: 0 und eine Meldung auf stderr ---
Print "fehlt " + LoadMesh("tests/assets/gibtsnicht.glb")
Print "kaputt " + LoadMesh("tests/assets/test_gltf_kaputt.gltf")
Print "draco " + LoadAnimMesh("tests/assets/test_gltf_draco.gltf")
