; 3D-24 - glTF Morph Targets: Gewichte aus Knoten und Netz, der
; Animationskanal "weights", Morph vor Skinning, Kopie, LoadMesh.
;
; Blitz3D kennt keine Morph Targets; die Erwartungen sind von Hand aus der
; Datei gerechnet (scripts/make_gltf_asset.py, test_gltf_morph.glb). Das
; Quadrat -1..1 liegt bei z = 0, die Kamera bei z = -5: 32 Pixel je
; Einheit, Mitte 160,120. Geprueft wird an Punkten, die mindestens 0.2
; Einheiten von jeder erwarteten Kante liegen: r = rot ("blatt"),
; g = gruen ("arm"), . = leer. Werte x1000 ueber Floor.

Graphics3D 320,240,0,2
SetBuffer BackBuffer()
cam = CreateCamera()
PositionEntity cam,0,0,-5
CameraClsColor cam,0,0,0

Function F$(x#)
	Return Int(Floor(x * 1000.0 + 0.5))
End Function
Function Stand$(e)
	Return "seq " + AnimSeq(e) + " len " + AnimLength(e) + " t " + F(AnimTime(e))
End Function
Function P$(x#, y#)
	px = Int(160 + x * 32) : py = Int(120 - y * 32)
	c = ReadPixelFast(px, py) And $FFFFFF
	rr = (c Shr 16) And 255 : gg = (c Shr 8) And 255
	If rr > 128 And gg < 64 Then Return "r"
	If gg > 128 And rr < 64 Then Return "g"
	Return "."
End Function
; Punkte als Liste "x,y x,y ..." -> Ergebnis je Punkt
Function Pruef$(punkte$)
	RenderWorld
	LockBuffer BackBuffer()
	s$ = ""
	While punkte <> ""
		sp = Instr(punkte, " ")
		If sp = 0 Then p$ = punkte : punkte = "" Else p$ = Left(punkte, sp - 1) : punkte = Mid(punkte, sp + 1)
		k = Instr(p, ",")
		s = s + " " + p + "=" + P(Float(Left(p, k - 1)), Float(Mid(p, k + 1)))
	Wend
	UnlockBuffer BackBuffer()
	Return s
End Function
Function V$(s, i)
	Return "v" + i + " " + F(VertexX(s,i)) + "," + F(VertexY(s,i)) + "," + F(VertexZ(s,i)) + " n " + F(VertexNX(s,i)) + "," + F(VertexNY(s,i)) + "," + F(VertexNZ(s,i))
End Function

m = LoadAnimMesh("tests/assets/test_gltf_morph.glb")
blatt = FindChild(m, "blatt") : arm = FindChild(m, "arm")
Print "0 " + Stand(m) + " | " + EntityClass(FindChild(m, "gelenk"))

; --- 1) "blatt": Gewichte aus der Animation "morphen" ---
HideEntity arm
; Bild 0: 0/0 - das Quadrat in Ruhe
Print "1a" + Pruef("0,0.8 0,1.2 0.8,0 1.2,0")
; Bild 5: hoch 0.5 - Oberkante bei 1.5
SetAnimTime m,5
Print "1b" + Pruef("0,1.3 0,1.7 1.2,0")
; Bild 9: hoch 0.9 - Oberkante bei 1.9
SetAnimTime m,9
Print "1c" + Pruef("0,1.7 0,2.1")
; Sequenz 1 bewegt "blatt" nicht: die Gewichte des Knotens, 0.5/0.25
SetAnimTime m,5,1
Print "1d" + Pruef("0,1.3 0,1.7 1.1,0 1.4,0")

; --- 2) "arm": Morph vor Skinning ---
ShowEntity arm : HideEntity blatt
; Sequenz 0 bewegt "arm" nicht: Gewichte des Netzes 0/1, x bis 2
SetAnimTime m,0
Print "2a" + Pruef("1.8,0 2.2,0 0,1.2")
; Sequenz 1, Bild 5: das Gelenk dreht -90 Grad um z. Erst der Morph (x bis
; 2), dann die Drehung (x -> -y): y reicht bis -2, x nur bis 1. Andersherum
; waere es wieder x bis 2.
SetAnimTime m,5,1
Print "2b" + Pruef("0,-1.8 0,-2.2 1.8,0 0,0.8")

; --- 3) Uebergang von Sequenz 1 zurueck zu 0: die Gewichte gleiten mit ---
ShowEntity blatt : HideEntity arm
SetAnimTime m,5,1
Animate m,1,0,0,4
UpdateWorld 2
; halb zwischen 0.5/0.25 (Knoten) und 0/0 (Bild 0 von "morphen"): 0.25/0.125
Print "3" + Pruef("0,1.1 0,1.4 1.05,0 1.2,0")
Animate m,0
FreeEntity m

; --- 4) Kopie: eigene Gewichte, eigene Knochen ---
m = LoadAnimMesh("tests/assets/test_gltf_morph.glb")
c = CopyEntity(m)
HideEntity m
HideEntity FindChild(c, "blatt")
SetAnimTime c,5,1
Print "4 kopie" + Pruef("0,-1.8 1.8,0")
ShowEntity m : HideEntity c
HideEntity FindChild(m, "blatt")
Print "4 original" + Pruef("0,-1.8 1.8,0")
FreeEntity c
FreeEntity m

; --- 5) LoadMesh: die Gewichte der Datei eingebacken ---
e = LoadMesh("tests/assets/test_gltf_morph.glb")
Print "5 kinder " + CountChildren(e) + " flaechen " + CountSurfaces(e)
For i = 1 To CountSurfaces(e)
	s = GetSurface(e, i)
	For k = 0 To CountVertices(s) - 1 : Print "5." + i + " " + V(s, k) : Next
Next
