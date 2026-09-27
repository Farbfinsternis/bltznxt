; 3D-24 - glTF mit Skinning und Animationen.
;
; Das Skinning hat einen gemessenen Anker: test_gltf_skin.glb ist der
; Streifen aus test_skin.b3d (gegen Blitz3D 11.8 gemessen, 3D-19) als glTF
; nachgebaut. Beide laufen durch dasselbe Szenario - Ruhelage, Bild 10, 20,
; 15, eine Kopie mit eigener Haltung, Animate, LoadMesh - und muessen
; dasselbe Bild ergeben. Gemeldet werden die Laeufe roter und gruener
; Pixel je Zeile, wie in test_3d19_animation_b3d.
;
; Die Knotenanimationen sind von Hand gerechnet: 60 Bilder je Sekunde,
; jede Sequenz beginnt bei ihrem fruehesten Schluessel, blitz = (x,y,-z).
; Die Dateien erzeugt scripts/make_gltf_asset.py. Werte x1000 ueber Floor.

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
Function Baum(e, tiefe)
	s$ = ""
	For i = 1 To tiefe : s = s + " " : Next
	v$ = ""
	If EntityClass(e) = "Mesh"
		For i = 1 To CountSurfaces(e)
			v = v + " " + CountVertices(GetSurface(e,i)) + "/" + CountTriangles(GetSurface(e,i))
		Next
	EndIf
	Print s + "[" + EntityName(e) + "] " + EntityClass(e) + " kinder " + CountChildren(e) + v + " | " + L(e)
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

; --- 1) Skinning: .b3d und glTF im selben Szenario ---
Dim erg$(7)
Function Szenario(datei$)
	m = LoadAnimMesh(datei)
	EntityFX m,1
	erg(0) = Bild()
	SetAnimTime m,10 : erg(1) = Bild()
	SetAnimTime m,20 : erg(2) = Bild()
	SetAnimTime m,15 : erg(3) = Bild()
	c = CopyEntity(m)
	SetAnimTime c,10
	PositionEntity m,0,-3,0
	erg(4) = Bild()
	FreeEntity c
	PositionEntity m,0,0,0
	Animate m,1,5
	For i = 1 To 3 : UpdateWorld : Next
	erg(5) = Stand(m) + " | " + Bild()
	FreeEntity m
	e = LoadMesh(datei)
	EntityFX e,1
	erg(6) = "kinder " + CountChildren(e) + " w " + F(MeshWidth(e)) + " h " + F(MeshHeight(e)) + " | " + Bild()
	FreeEntity e
End Function

Dim b3d$(7)
Szenario("tests/assets/test_skin.b3d")
For i = 0 To 6 : b3d(i) = erg(i) : Next
Szenario("tests/assets/test_gltf_skin.glb")
For i = 0 To 6
	If erg(i) = b3d(i) Then g$ = "= b3d" Else g$ = "ANDERS als b3d: " + b3d(i)
	Print "1." + i + " " + erg(i) + " " + g
Next

; --- 2) Der Baum der glTF-Datei, Gelenke als Pivot ---
m = LoadAnimMesh("tests/assets/test_gltf_skin.glb")
Baum(m, 0)
k1 = FindChild(m, "knochen1") : k2 = FindChild(m, "knochen2")
Print "2 " + Stand(m)
SetAnimTime m,10
Print "2a bild 10 k2 " + W(k2)
SetAnimTime m,19
Print "2b bild 19 k1 " + L(k1) + " k2 " + W(k2)

; --- 3) zweite Sequenz aus derselben Datei: knochen1 zurueck in Ruhe ---
SetAnimTime m,3,1
Print "3 " + Stand(m) + " k1 " + L(k1) + " k2 " + L(k2)
Animate m,3,1,1
Print "3a " + Stand(m)
FreeEntity m

; --- 4) Knotenanimation: Beginn bei 0.5 s, STEP, CUBICSPLINE ---
; Die Skalierung der Kiste steht in der Welt-y des Ankers (lokal 0,1,0).
n = LoadAnimMesh("tests/assets/test_gltf_knoten.gltf")
Baum(n, 0)
k = FindChild(n, "kiste") : a = FindChild(n, "anker")
Print "4 " + Stand(n)
For t = 0 To 30 Step 5
	SetAnimTime n,t
	Print "4 fahren " + t + ": " + L(k) + " anker " + W(a)
Next
SetAnimTime n,14 : Print "4 step 14: " + F(EntityY(a,True))
SetAnimTime n,15 : Print "4 step 15: " + F(EntityY(a,True))
SetAnimTime n,29 : Print "4 step 29: " + F(EntityY(a,True))
For t = 0 To 9 Step 3
	SetAnimTime n,t,1
	Print "4 drehen " + t + ": " + L(k) + " anker " + W(a) + " | " + Stand(n)
Next

; --- 5) LoadAnimSeq aus einer zweiten Datei, zugeordnet ueber den Namen ---
q = LoadAnimSeq(n, "tests/assets/test_gltf_knoten_seq.glb")
SetAnimTime n,6,q
Print "5 seq=" + q + " " + Stand(n) + " kiste " + L(k)
FreeEntity n

; --- 6) Kopie mit Knochen: die Kopie biegt sich, das Original nicht
;        (Bild 20 waere wie im Original wieder Bild 0 - fmod ueber die Laenge)
m = LoadAnimMesh("tests/assets/test_gltf_skin.glb")
c = CopyEntity(m)
SetAnimTime c,10
kc = FindChild(c, "knochen2")
Print "6 kopie k2 " + W(kc) + " original k2 " + W(FindChild(m, "knochen2"))
