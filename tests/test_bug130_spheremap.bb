; BUG-130 - Texturflag 64 (Sphere-Map) bildet die Umgebung ab.
;
; Wie D3DTSS_TCI_CAMERASPACENORMAL mit der sphere_mat des Originals:
; u = n.x/2 + 1/2, v = -n.y/2 + 1/2 aus der Normalen im Kameraraum. Je nach
; Drehung des Wuerfels liegt also ein anderes Farbfeld der Textur in der
; Bildmitte. Gemessen an Blitz3D 11.8 (build/bug130_20261025): die Erwartung
; ist die Ausgabe des Originals. Bis dahin lag die Textur wie eine gewoehnliche
; auf dem Wuerfel, und die Chromkanne in samples/mak/teapot war fast
; unsichtbar.
Graphics3D 320,240,0,2
cam=CreateCamera()
cube=CreateCube()
PositionEntity cube,0,0,4
EntityFX cube,1
; links rot, rechts gruen
lr=CreateTexture(64,64,1+64)
SetBuffer TextureBuffer(lr)
Color 255,0,0 : Rect 0,0,32,64
Color 0,255,0 : Rect 32,0,32,64
; oben blau, unten gelb
ou=CreateTexture(64,64,1+64)
SetBuffer TextureBuffer(ou)
Color 0,0,255 : Rect 0,0,64,32
Color 255,255,0 : Rect 0,32,64,32
SetBuffer BackBuffer()
For i=0 To 3
	If i<2 Then EntityTexture cube,lr Else EntityTexture cube,ou
	If i=0 Then RotateEntity cube,0,40,0
	If i=1 Then RotateEntity cube,0,-40,0
	If i=2 Then RotateEntity cube,40,0,0
	If i=3 Then RotateEntity cube,-40,0,0
	RenderWorld
	LockBuffer BackBuffer()
	c=ReadPixelFast(160,120) And $FFFFFF
	UnlockBuffer BackBuffer()
	Print ((c Shr 16) And 255)+","+((c Shr 8) And 255)+","+(c And 255)
Next
