; BUG-176 - Ein Pixel, dessen Mitte genau auf einer rechten Kante liegt,
; gehoert nicht dazu. Direct3D 7 laesst ihn weg; in OpenGL entscheidet die
; Fuellregel mit dem Ursprung unten andersherum.
;
; Der Fall tritt auf, sobald eine Kante genau durch Pixelmitten laeuft. Die
; Geometrie hier ist die des MD2-Tests ohne dessen Ausweich-Versatz der Kamera:
; ein rechtwinkliges Dreieck (-5,-5) (5,-5) (-5,5) bei z=0, Kamera auf
; (0,0,-10), Fenster 200x200 - die Hypotenuse laeuft als 45-Grad-Diagonale
; genau durch die Pixelmitten. Gemessen am Original (2026-09-22,
; build/kante20260922): jede Zeile endet dort einen Pixel frueher als bei uns,
; und die oberste Zeile faellt ganz weg.
;
; Waagerecht wird deshalb wie senkrecht (BUG-152) um knapp einen halben Pixel
; verschoben: 1/2 - 1/256, damit keine Kante genau auf einer Pixelmitte landet.

Graphics3D 200,200,0,2
SetBuffer BackBuffer()
cam = CreateCamera()
CameraClsColor cam,0,0,0
PositionEntity cam,0,0,-10

m = CreateMesh()
sf = CreateSurface(m)
AddVertex sf,-5,-5,0 : AddVertex sf,5,-5,0 : AddVertex sf,-5,5,0
AddTriangle sf,0,2,1
EntityFX m,1 : EntityColor m,255,0,0
RenderWorld
For y = 48 To 152 Step 4
	Print "diagonale als rechte kante, zeile " + y + ": " + Zeile(y)
Next

; Dasselbe Dreieck gespiegelt: die Diagonale ist jetzt die linke Kante.
FreeEntity m
m = CreateMesh()
sf = CreateSurface(m)
AddVertex sf,5,5,0 : AddVertex sf,-5,5,0 : AddVertex sf,5,-5,0
AddTriangle sf,0,2,1
EntityFX m,1 : EntityColor m,0,255,0
RenderWorld
For y = 48 To 152 Step 4
	Print "diagonale als linke kante, zeile " + y + ": " + Zeile(y)
Next

Function Zeile$(y)
	LockBuffer BackBuffer()
	l = -1 : r = -1
	For i = 0 To 199
		If (ReadPixelFast(i,y) And $FFFFFF) <> 0
			If l < 0 Then l = i
			r = i
		EndIf
	Next
	UnlockBuffer BackBuffer()
	Return l + "-" + r
End Function
