; BUG-200 - DrawBlock, DrawImageRect und DrawBlockRect beachten auf dem
; Bildschirm den Griffpunkt (gxCanvas::blit zieht handle_x/y ab). Werte am
; Original gemessen (2026-10-04): 1 = Pixel gesetzt.

Graphics 320, 240, 32, 2
SetBuffer BackBuffer()

img = CreateImage(20, 10)
SetBuffer ImageBuffer(img)
Color 255, 0, 0 : Rect 0, 0, 20, 10, 1
SetBuffer BackBuffer()
MidHandle img
For m = 0 To 3
	ClsColor 0, 0, 0 : Cls
	Select m
		Case 0 : DrawImage img, 100, 100
		Case 1 : DrawBlock img, 100, 100
		Case 2 : DrawImageRect img, 100, 100, 0, 0, 20, 10
		Case 3 : DrawBlockRect img, 100, 100, 5, 2, 10, 6
	End Select
	s$ = "M" + m
	For k = 0 To 5
		px = 85 + k * 5
		s$ = s$ + " " + ((ReadPixel(px, 100) And $FFFFFF) > 0)
	Next
	For k = 0 To 4
		py = 92 + k * 4
		s$ = s$ + " " + ((ReadPixel(100, py) And $FFFFFF) > 0)
	Next
	Print s$
Next
End
