; BUG-209 - Oval und Rect auf dem Bildschirm wie im Original.
;
; Der Bildschirm zeichnet jetzt mit demselben Raster wie der Bildpuffer
; (gxCanvas::oval/rect); vorher war Oval 1 Pixel zu breit und zu hoch, und
; Rect mit negativer Breite zeichnete ein Rechteck. Jede Zeile ist eine
; Signatur der gesetzten Pixel (Anzahl, Rahmen, Pruefsumme), die Erwartung
; ist die Ausgabe von Blitz3D 11.8 (Review-Probe g2e). Ausgelassen: die
; beiden Text-Zeilen, deren Abweichung die Standardschrift ist (BUG-139).
Graphics 160, 120, 0, 2
SetBuffer BackBuffer()
Function Sig$(name$)
	n = 0 : x0 = 999 : y0 = 999 : x1 = -1 : y1 = -1 : h = 0
	LockBuffer BackBuffer()
	For y = 0 To 119
		For x = 0 To 159
			c = ReadPixelFast(x, y) And $FFFFFF
			If c <> 0 Then
				n = n + 1 : h = (h * 31 + x * 977 + y * 131 + c) And $FFFFFF
				If x < x0 Then x0 = x
				If y < y0 Then y0 = y
				If x > x1 Then x1 = x
				If y > y1 Then y1 = y
			EndIf
		Next
	Next
	UnlockBuffer BackBuffer()
	Cls
	Return name + " n=" + n + " box=" + x0 + "," + y0 + "-" + x1 + "," + y1 + " h=" + h
End Function
ClsColor 0, 0, 0 : Cls : Color 255, 255, 255
Plot 5, 5 : Print Sig("plot")
Line 0, 0, 159, 119 : Print Sig("line diag")
Line 159, 0, 0, 119 : Print Sig("line anti")
Line 10, 10, 50, 13 : Print Sig("line flach")
Line 10, 10, 13, 50 : Print Sig("line steil")
Line 50, 13, 10, 10 : Print Sig("line flach rueck")
Line 140, 10, 140, 10 : Print Sig("line punkt")
Rect 10, 10, 30, 20, 0 : Print Sig("rect offen")
Rect 50, 10, 30, 20, 1 : Print Sig("rect voll")
Rect 120, 10, 0, 5, 1 : Print Sig("rect w0")
Rect 130, 10, -5, 5, 1 : Print Sig("rect w-5")
Oval 10, 40, 31, 21, 0 : Print Sig("oval offen")
Oval 50, 40, 31, 21, 1 : Print Sig("oval voll")
Oval 90, 40, 1, 1, 1 : Print Sig("oval 1")
Oval 100, 40, 2, 2, 1 : Print Sig("oval 2")
Oval 110, 40, 3, 3, 0 : Print Sig("oval 3 offen")
Oval 10, 10, 40, 40, 1 : Print Sig("kreis 40")
Oval 10, 10, 41, 41, 0 : Print Sig("kreis 41 offen")
Origin 5, 5 : Rect 0, 70, 10, 10, 1 : Origin 0, 0 : Print Sig("origin rect")
Viewport 20, 90, 20, 20 : Rect 0, 0, 160, 120, 1 : Viewport 0, 0, 160, 120 : Print Sig("viewport")
img = CreateImage(8, 8)
SetBuffer ImageBuffer(img) : Cls : Color 255, 128, 0 : Rect 2, 2, 4, 4, 1 : SetBuffer BackBuffer()
DrawImage img, 120, 90 : Print Sig("drawimage")
MaskImage img, 255, 128, 0 : DrawImage img, 130, 90 : Print Sig("drawimage maske")
DrawBlock img, 140, 90 : Print Sig("drawblock")
HandleImage img, 4, 4 : DrawBlock img, 150, 100 : Print Sig("drawblock handle")
Color 255, 255, 255
Line -10, 50, 200, 60 : Print Sig("line clip")
Oval -10, -10, 30, 30, 1 : Print Sig("oval clip")
