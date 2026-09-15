; BUG-91 - Punkt- und Spotlicht schwaechen mit Range/Abstand ab, je Vertex.
;
; gxLight (gxruntime/gxlight.cpp) setzt dvAttenuation1 = 1/range und laesst die
; anderen Faktoren auf 0; die feste Direct3D-7-Pipeline rechnet damit
; range/Abstand, je Vertex, und interpoliert. Bis 2026-09-15 rechneten wir
; 1 - Abstand/range je Bildpunkt: zu dunkel bis schwarz (A), und auf grossen
; Flaechen Lichtflecken, wo das Original die weit entfernten Ecken
; interpoliert und gleichmaessig dunkel bleibt (B, C).
;
; Die erwarteten Werte sind am Original gemessen (F:\dev\Blitz3D, 2026-09-15),
; dieselbe Logik schrieb dort in eine Datei.

Graphics3D 320,240,32,2
AmbientLight 0,0,0
cam = CreateCamera()
CameraClsColor cam,0,0,0

; A) Wuerfel, Punktlicht im Ursprung, verschiedene Reichweiten
p = CreateLight(2)
cube = CreateCube()
PositionEntity cube,0,0,4
EntityColor cube,100,100,100
Data 20, 5, 3.5, 2
For k = 1 To 4
  Read r#
  LightRange p, r
  UpdateWorld : RenderWorld
  LockBuffer BackBuffer()
  Zeile "A range " + Int(r*10) + ": mitte " + K(160,120) + " halb " + K(185,120)
  UnlockBuffer BackBuffer()
  Flip
Next
FreeEntity cube

; B) grosse Flaeche nah am Punktlicht - je Vertex gegen je Bildpunkt
wand = CreateCube()
ScaleEntity wand,4,4,0.1
PositionEntity wand,0,0,3
EntityColor wand,200,200,200
PositionEntity p,0,0,1.5
LightRange p,2
UpdateWorld : RenderWorld
LockBuffer BackBuffer()
z$ = "B punkt nah:"
For x = 20 To 300 Step 40
  z = z + " " + K(x,120)
Next
Zeile z
UnlockBuffer BackBuffer()
Flip
FreeEntity p

; C) Spotlicht von der Kamera auf dieselbe Flaeche
s = CreateLight(3)
LightRange s,4
LightConeAngles s,20,60
UpdateWorld : RenderWorld
LockBuffer BackBuffer()
z$ = "C spot:"
For x = 20 To 300 Step 40
  z = z + " " + K(x,120)
Next
Zeile z
UnlockBuffer BackBuffer()
Flip
Ende()
End

Function K(x, y)
  Return (ReadPixelFast(x, y) Shr 8) And 255
End Function
Function Zeile(s$)
  Print s
End Function
Function Ende()
End Function
