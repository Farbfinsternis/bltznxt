; BUG-229: glTF doubleSided heisst Rueckseite zeichnen UND mit umgedrehter
; Normale beleuchten. Bis dahin setzte der Lader nur FX 16 - von hinten
; gesehen blieb die Flaeche dunkel, obwohl das Licht von dieser Seite kam.
; FX 16 allein dreht nichts um, wie im Original: das Vergleichsviereck
; bleibt von hinten dunkel.
;
; Viereck in der glTF-Ebene z = 0, Normale +Z -> in Blitz zeigt die
; Vorderseite nach -Z. Kamera bei +Z schaut nach -Z auf die Rueckseite,
; das Richtungslicht kommt von der Kameraseite. Kein Umgebungslicht.

Graphics3D 320,240,0,2
SetBuffer BackBuffer()
AmbientLight 0,0,0
cam = CreateCamera()
CameraClsColor cam,0,0,0
PositionEntity cam,0,0,5
RotateEntity cam,0,180,0          ; schaut nach -Z
licht = CreateLight(1)
RotateEntity licht,0,180,0        ; Licht laeuft nach -Z, kommt also von der Kamera

Function Hell(x, y)
	LockBuffer BackBuffer()
	rgb = ReadPixelFast(x, y, BackBuffer())
	UnlockBuffer BackBuffer()
	Return (rgb Shr 16) And 255
End Function

; 1. glTF, doubleSided
q = LoadMesh("tests/assets/test_gltf_zweiseitig.gltf")
RenderWorld
r = Hell(160, 120)
If r > 200 Then Print "glTF Rueckseite hell" Else Print "glTF Rueckseite dunkel: " + r
FreeEntity q

; 2. dasselbe Viereck von Hand mit FX 16: zu sehen, aber dunkel wie im Original
m = CreateMesh()
s = CreateSurface(m)
v0 = AddVertex(s,-1,-1,0) : v1 = AddVertex(s,1,-1,0) : v2 = AddVertex(s,1,1,0) : v3 = AddVertex(s,-1,1,0)
For i = 0 To 3 : VertexNormal s,i,0,0,-1 : Next
AddTriangle s,0,2,1 : AddTriangle s,0,3,2
EntityFX m,16
RenderWorld
r = Hell(160, 120)
If r < 40 Then Print "FX 16 Rueckseite dunkel" Else Print "FX 16 Rueckseite hell: " + r
; leerer Rand zur Kontrolle
Print "Rand " + Hell(5, 5)
End
