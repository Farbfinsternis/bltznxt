; 3D-19 - .b3d: Knoten, Knochen, Gewichte, Schluessel. LoadAnimMesh,
; LoadAnimSeq, LoadMesh (eingeschmolzen), CopyEntity eines Netzes mit
; Knochen. Das Skinning wird am Bild gemessen: der Streifen ist links rot,
; rechts gruen, vollhell auf Schwarz; je Zeile werden die Laeufe gemeldet.
;
; Die Dateien erzeugt scripts/make_b3d_asset.py. Gemessen am Original
; (2026-09-26). Werte x1000 ueber Floor.

Graphics3D 320,240,0,2
SetBuffer BackBuffer()
cam = CreateCamera()
PositionEntity cam,0.013,0.031,-5.37   ; keine Kante genau auf Pixelmitten
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
Function Stand$(e)
	Return "seq " + AnimSeq(e) + " len " + AnimLength(e) + " t " + F(AnimTime(e)) + " an " + Animating(e)
End Function
Function Z(s$)
	Print s
End Function
Function Baum(e, tiefe)
	s$ = ""
	For i = 1 To tiefe : s = s + " " : Next
	v$ = ""
	If EntityClass(e) = "Mesh"
		For i = 1 To CountSurfaces(e)
			v = v + " " + CountVertices(GetSurface(e,i)) + "/" + CountTriangles(GetSurface(e,i))
		Next
	EndIf
	Z s + "[" + EntityName(e) + "] " + EntityClass(e) + " kinder " + CountChildren(e) + v + " | " + L(e) + " | " + Stand(e)
	For i = 1 To CountChildren(e)
		Baum(GetChild(e,i), tiefe + 1)
	Next
End Function
; Laeufe roter (r) und gruener (g) Punkte in Zeile y
Function Zeile$(y)
	s$ = ""
	art$ = "." : von = 0
	For x = 0 To 320
		a$ = "."
		If x < 320
			p = ReadPixelFast(x,y) And $FFFFFF
			rr = (p Shr 16) And 255 : gg = (p Shr 8) And 255
			If rr > 128 And gg < 64 Then a = "r"
			If gg > 128 And rr < 64 Then a = "g"
		EndIf
		If a <> art
			If art <> "." Then s = s + " " + art + von + "-" + (x - 1)
			art = a : von = x
		EndIf
	Next
	Return "y" + y + ":" + s
End Function
Function Bild$()
	RenderWorld
	LockBuffer BackBuffer()
	s$ = Zeile(40) + " |" + Zeile(90) + " |" + Zeile(120) + " |" + Zeile(135)
	UnlockBuffer BackBuffer()
	Return s + " | tris " + TrisRendered()
End Function

; --- 1) Streifen mit zwei Knochen
m = LoadAnimMesh("tests/assets/test_skin.b3d")
EntityFX m,1
Baum(m, 0)
k1 = FindChild(m, "knochen1") : k2 = FindChild(m, "knochen2")
Z "1 " + Bild()

; --- 2) Bild 10 und 20
SetAnimTime m,10
Z "2a k2 " + W(k2) + " | " + Bild()
SetAnimTime m,20
Z "2b k1 " + L(k1) + " k2 " + W(k2) + " | " + Bild()
SetAnimTime m,15
Z "2c " + Bild()

; --- 3) Vertexdaten bleiben die Ruhelage
s = GetSurface(m,1)
Z "3 v0 " + F(VertexX(s,0)) + "," + F(VertexY(s,0)) + " n " + F(VertexNX(s,0)) + "," + F(VertexNY(s,0)) + "," + F(VertexNZ(s,0))

; --- 4) Kopie mit eigener Haltung, das Original bewegt man weg
c = CopyEntity(m)
Z "4a kopie " + Stand(c) + " kinder " + CountChildren(c)
SetAnimTime c,10
PositionEntity m,0,-3,0
Z "4b " + Bild()
FreeEntity c
PositionEntity m,0,0,0

; --- 5) laufen lassen
Animate m,1,5
For i = 1 To 3 : UpdateWorld : Next
Z "5 " + Stand(m) + " | " + Bild()
Animate m,0
HideEntity m

; --- 6) LoadMesh schmilzt ein: Ruhelage, ohne Knochen
e = LoadMesh("tests/assets/test_skin.b3d")
EntityFX e,1
Z "6 kinder " + CountChildren(e) + " fl " + CountSurfaces(e) + " w " + F(MeshWidth(e)) + " h " + F(MeshHeight(e)) + " " + Stand(e) + " | " + Bild()
FreeEntity e

; --- 7) Hierarchie ohne Knochen
n = LoadAnimMesh("tests/assets/test_nodes.b3d")
Baum(n, 0)
a = FindChild(n, "a") : b = FindChild(n, "b") : cc = FindChild(n, "c")
sa = GetSurface(a,1)
Z "7a farbe " + VertexRed(sa,0) + "," + VertexGreen(sa,0) + "," + VertexBlue(sa,0) + "," + F(VertexAlpha(sa,0)) + " uv " + F(VertexU(sa,1,0)) + "," + F(VertexV(sa,1,0)) + " uv1 " + F(VertexU(sa,1,1)) + "," + F(VertexV(sa,1,1)) + " n " + F(VertexNZ(sa,0))
tex = GetBrushTexture(GetSurfaceBrush(sa))
Z "7b textur " + TextureWidth(tex) + "x" + TextureHeight(tex)
sc = GetSurface(cc,1)
Z "7c c uv1 " + F(VertexU(sc,1,1)) + "," + F(VertexV(sc,1,1))
SetAnimTime n,5
Z "7d a " + L(a) + " b " + L(b) + " c " + W(cc)

; --- 8) zweite Sequenz
q = LoadAnimSeq(n, "tests/assets/test_nodes_seq.b3d")
SetAnimTime n,3,q
Z "8 seq=" + q + " " + Stand(n) + " a " + L(a) + " b " + L(b)

; --- 9) LoadMesh derselben Hierarchie
e = LoadMesh("tests/assets/test_nodes.b3d")
Z "9 fl " + CountSurfaces(e) + " w " + F(MeshWidth(e)) + " h " + F(MeshHeight(e)) + " d " + F(MeshDepth(e))
