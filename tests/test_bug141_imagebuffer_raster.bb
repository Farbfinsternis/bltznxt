; BUG-141 - 2D-Befehle zeichnen in den Puffer, den SetBuffer gesetzt hat:
; Pixeltabellen fuer einen ImageBuffer.
;
; Jede Tabelle ist am Original gemessen (2026-09-17). Die Algorithmen folgen
; gxruntime/gxcanvas.cpp: Plot, Line samt Clipping am Viewport, Rect, Oval,
; Cls, Origin und Viewport je Puffer, SetBuffer setzt beide zurueck, DrawImage
; und DrawBlock mit Handle und Maskenfarbe, TileImage. Vorher zeichneten alle
; diese Befehle in den Backbuffer.

Graphics3D 320,240,0,2
Global img

; 24x24-Image als Zeichen ausgeben: . = 0, sonst Kennbuchstabe der Farbe
Function Dump(name$)
	Print "== " + name
	For y = 0 To 23
		s$ = ""
		For x = 0 To 23
			v = ReadPixel(x, y, ImageBuffer(img))
			c = v And $FFFFFF
			Select c
				Case 0 : s = s + "."
				Case $FF0000 : s = s + "R"
				Case $00FF00 : s = s + "G"
				Case $0000FF : s = s + "B"
				Case $FFFFFF : s = s + "W"
				Case $FFFF00 : s = s + "Y"
				Default : s = s + "?"
			End Select
		Next
		Print s
	Next
End Function

Function Neu()
	If img Then FreeImage img
	img = CreateImage(24, 24)
	SetBuffer ImageBuffer(img)
End Function

; Grundwert eines frischen Image
Neu()
Print "frisch readpixel " + Hex(ReadPixel(3,3,ImageBuffer(img)))

Neu()
Color 255,0,0 : Plot 0,0 : Plot 23,23 : Plot 24,5 : Plot -1,5 : Plot 5,5
Dump "plot"

Neu()
Color 255,0,0 : Line 0,0,23,10
Color 0,255,0 : Line 23,23,2,0
Color 0,0,255 : Line 5,20,5,20
Color 255,255,255 : Line -5,12,30,14
Dump "line"

Neu()
Color 255,0,0 : Rect 1,1,6,4,1
Color 0,255,0 : Rect 10,1,6,4,0
Color 0,0,255 : Rect 20,20,10,10,1
Color 255,255,255 : Rect -2,10,5,1,0
Color 255,255,0 : Rect 12,10,1,1,0
Dump "rect"

Neu()
Color 255,0,0 : Oval 0,0,9,9,1
Color 0,255,0 : Oval 12,0,10,7,0
Color 0,0,255 : Oval 0,12,6,11,0
Color 255,255,255 : Oval 12,12,11,11,1
Color 255,255,0 : Oval 20,20,8,8,1
Dump "oval"

Neu()
Color 255,0,0 : Rect 0,0,24,24,1
Origin 3,2
Viewport 5,5,10,8
ClsColor 0,0,255 : Cls
Color 0,255,0 : Rect 0,0,30,30,0
Color 255,255,255 : Line 0,0,20,20
Color 255,255,0 : Oval 4,4,12,12,1
Dump "origin viewport cls"

Neu()
Color 255,0,0 : Rect 0,0,24,24,1
Origin 3,2 : Viewport 5,5,10,8
SetBuffer ImageBuffer(img)
Color 0,255,0 : Rect 0,0,4,4,1
Dump "setbuffer setzt origin und viewport zurueck"

; DrawImage / DrawBlock in das Image, mit Maske und Handle
src = CreateImage(6,6)
SetBuffer ImageBuffer(src)
Color 0,0,255 : Rect 0,0,6,6,1
Color 0,0,0 : Rect 2,2,2,2,1
Neu()
Color 255,0,0 : Rect 0,0,24,24,1
DrawImage src,1,1
DrawBlock src,10,1
HandleImage src,3,3
DrawImage src,4,15
DrawBlock src,15,15
Dump "drawimage drawblock"

Neu()
TileImage src,0,0
Dump "tileimage"

End
