; 3D-24 - glTF KHR_texture_transform: Versatz, Drehung, texCoord aus der
; Erweiterung, und PositionTexture obendrauf.
;
; Blitz3D kennt glTF nicht; die Erwartungen sind von Hand gerechnet
; (scripts/make_gltf_asset.py, test_gltf_uvtrafo.glb): uv' = T * R * S * uv,
; R = [cos sin; -sin cos]. Die Textur ist 2x2 - oben blau | rot, unten
; gruen | weiss -, gemessen wird an den Texelmitten (u,v je 0.25 / 0.75).
; Die Tafeln stehen bei z = 6: 26.67 Pixel je Einheit, Mitte 160,120.

Graphics3D 320,240,0,2
SetBuffer BackBuffer()
cam = CreateCamera()
CameraClsColor cam,0,0,0

Function Farbe$(x#, y#)
	c = ReadPixelFast(Int(160 + x * 160.0 / 6.0), Int(120 - y * 160.0 / 6.0)) And $FFFFFF
	r = (c Shr 16) And 255 : g = (c Shr 8) And 255 : b = c And 255
	If r > 200 And g > 200 And b > 200 Then Return "weiss"
	If r > 200 And g < 60 And b < 60 Then Return "rot"
	If g > 200 And r < 60 And b < 60 Then Return "gruen"
	If b > 200 And r < 60 And g < 60 Then Return "blau"
	Return r + "," + g + "," + b
End Function
; die vier Texelmitten einer Tafel: links oben, rechts oben, links unten, rechts unten
Function Tafel$(cx#)
	Return Farbe(cx - 0.5, 0.5) + " " + Farbe(cx + 0.5, 0.5) + " " + Farbe(cx - 0.5, -0.5) + " " + Farbe(cx + 0.5, -0.5)
End Function
Function Alle()
	RenderWorld
	LockBuffer BackBuffer()
	Print " ohne:    " + Tafel(-4.5)
	Print " versatz: " + Tafel(-1.5)
	Print " drehung: " + Tafel(1.5)
	Print " satz:    " + Tafel(4.5)
	UnlockBuffer BackBuffer()
End Function

m = LoadAnimMesh("tests/assets/test_gltf_uvtrafo.glb")
t = GetChild(m, 1)
Print "flaechen " + CountSurfaces(t)
Print "1) aus der Datei"
Alle()

; PositionTexture wirkt nach der Matrix aus der Datei: 0.5 hebt den
; glTF-Versatz der zweiten Tafel genau auf.
tex = GetBrushTexture(GetSurfaceBrush(GetSurface(t, 2)))
PositionTexture tex, 0.5, 0
Print "2) versatz + PositionTexture 0.5"
Alle()
