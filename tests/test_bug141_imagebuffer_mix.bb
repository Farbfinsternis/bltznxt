; BUG-141 - ImageBuffer im Zusammenspiel: ein ins Image gezeichnetes Bild auf
; dem Bildschirm (DrawImage maskiert, DrawBlock nicht), ReadPixel/WritePixel
; mit Origin und Viewport des Puffers (ausserhalb liefert ReadPixel die
; Maskenfarbe), LockBuffer nach dem Zeichnen, Text in ein Image (schwarzer
; Text wird zu $000010) und MaskImage beim Zeichnen in ein anderes Image.
; Alle Werte am Original gemessen (2026-09-17).

Graphics3D 320,240,0,2
img = CreateImage(16,16)
SetBuffer ImageBuffer(img)
Color 255,0,0 : Rect 0,0,8,16,1
; 1) ins Image gezeichnet, dann auf den Bildschirm
SetBuffer BackBuffer()
ClsColor 0,0,255 : Cls
DrawImage img,50,50
Print "1 bildschirm rot " + Hex(ReadPixel(52,52)) + " maskiert " + Hex(ReadPixel(60,52))
DrawBlock img,80,50
Print "1b drawblock schwarz " + Hex(ReadPixel(90,52))
; 2) ReadPixel/WritePixel mit Origin und Viewport im Image
SetBuffer ImageBuffer(img)
Origin 2,3
Viewport 4,4,6,6
WritePixel 5,5,$FF00FF00
WritePixel 0,0,$FF00FF00
Origin 0,0
Print "2 writepixel mit origin " + Hex(ReadPixel(7,8)) + " ausserhalb viewport " + Hex(ReadPixel(2,3)) + " readpixel ausserhalb viewport " + Hex(ReadPixel(0,0))
MaskImage img,255,0,255
Print "2b ausserhalb nach maskimage " + Hex(ReadPixel(0,0))
SetBuffer BackBuffer()
; 3) LockBuffer nach Zeichnen
LockBuffer ImageBuffer(img)
Print "3 lock readpixelfast " + Hex(ReadPixelFast(1,1,ImageBuffer(img))) + " " + Hex(ReadPixelFast(7,8,ImageBuffer(img)))
WritePixelFast 12,12,$FFFFFF00,ImageBuffer(img)
UnlockBuffer ImageBuffer(img)
Print "3b nach unlock " + Hex(ReadPixel(12,12,ImageBuffer(img)))
; 4) Text ins Image: werden Pixel gesetzt, und in welcher Farbe
t = CreateImage(64,20)
SetBuffer ImageBuffer(t)
Color 255,255,255 : Text 0,0,"Hi"
n = 0
For y = 0 To 19 : For x = 0 To 63
	If (ReadPixel(x,y) And $FFFFFF) = $FFFFFF Then n = n + 1
Next : Next
Print "4 text pixel gesetzt " + (n > 0)
Color 0,0,0 : Cls : Text 0,0,"Hi"
n = 0
For y = 0 To 19 : For x = 0 To 63
	If (ReadPixel(x,y) And $FFFFFF) = $10 Then n = n + 1
Next : Next
Print "4b schwarzer text als 000010 " + (n > 0)
SetBuffer BackBuffer()
; 5) maskiertes Image in ein anderes Image
src = CreateImage(4,4)
SetBuffer ImageBuffer(src)
Color 255,0,255 : Rect 0,0,4,4,1 : Color 0,255,0 : Plot 1,1
MaskImage src,255,0,255
dst = CreateImage(8,8)
SetBuffer ImageBuffer(dst)
ClsColor 255,255,0 : Cls
DrawImage src,2,2
Print "5 maske " + Hex(ReadPixel(2,2)) + " punkt " + Hex(ReadPixel(3,3))
End
