; BUG-66 - EntityShininess: das Glanzlicht wird je Vertex gerechnet.
;
; Das Original zeichnet ueber die feste Direct3D-7-Pipeline: Licht je Vertex,
; interpoliert. gxScene setzt aus EntityShininess s die Staerke min(s,1) und
; den Exponenten s*128, gedeckelt bei 128; das Licht hat immer weisses
; Glanzlicht (gxLight), und das Ergebnis wird zur fertigen Farbe addiert.
; Bis 2026-09-15 rechneten wir je Bildpunkt und multiplizierten mit der
; Entityfarbe: der graue Wuerfel gab bei s=1 in der Mitte 128 statt 75.
;
; Die erwarteten Werte sind am Original gemessen (F:\dev\Blitz3D, 2026-09-15),
; dieselbe Logik schrieb dort in eine Datei. Das diffuse Licht bleibt bei uns
; bewusst je Bildpunkt; auf den ebenen Flaechen dieses Tests ist das dasselbe.

Graphics3D 320,240,32,2
AmbientLight 0,0,0
cam = CreateCamera()
CameraClsColor cam,0,0,0

; --- 1) Der gemessene Fall: Richtungslicht von vorn, grauer Wuerfel ---
licht = CreateLight(1)
cube = CreateCube()
PositionEntity cube,0,0,4
EntityColor cube,64,64,64
Data 0, 0.25, 0.5, 1, 2
For k = 1 To 5
  Read s#
  EntityShininess cube, s
  UpdateWorld : RenderWorld
  LockBuffer BackBuffer()
  Zeile "grau s=" + Int(s * 100) + "% mitte " + Kanal(160,120) + " halb " + Kanal(190,120)
  UnlockBuffer BackBuffer()
  Flip
Next

; --- 2) Farbiges Licht: das Glanzlicht bleibt weiss ---
LightColor licht,255,0,0
EntityColor cube,128,128,128
EntityShininess cube,0.5
UpdateWorld : RenderWorld
LockBuffer BackBuffer()
Zeile "rotlicht mitte " + Farbe(ReadPixelFast(160,120))
UnlockBuffer BackBuffer()
Flip

; --- 3) Flatshaded (EntityFX 4): dasselbe Glanzlicht, ohne Interpolation ---
LightColor licht,255,255,255
EntityColor cube,64,64,64
EntityShininess cube,0.25
EntityFX cube,4
UpdateWorld : RenderWorld
LockBuffer BackBuffer()
Zeile "flat mitte " + Kanal(160,120)
UnlockBuffer BackBuffer()
Flip
Ende()
End

Function Kanal(x, y)
  Return (ReadPixelFast(x, y) Shr 8) And 255
End Function

Function Farbe$(p)
  Return ((p Shr 16) And 255) + "," + ((p Shr 8) And 255) + "," + (p And 255)
End Function
Function Zeile(s$)
  Print s
End Function
Function Ende()
End Function
