; BUG-222 - TileBlock/TileImage auf dem Bildschirm: Versatz, Origin, Viewport, Handle.
;
; Vorher waren x- und y-Versatz im Bildschirmpfad vertauscht (Games/wing_ring
; rollte waagerecht statt senkrecht). Die Kachel hat oben eine rote Zeile und
; links eine gruene Spalte; je Fall stehen die Zeilen mit Rot in Spalte 8 und
; die Spalten mit Gruen in Zeile 4.
; Erwartung: Ausgabe von Blitz3D 11.8.
Graphics 200,150,0,2
SetBuffer BackBuffer()
img=CreateImage(16,8)
SetBuffer ImageBuffer(img)
Color 0,0,255 : Rect 0,0,16,8
Color 255,0,0 : Rect 0,0,16,1
Color 0,255,0 : Rect 0,0,1,8
SetBuffer BackBuffer()

Cls : TileBlock img,0,3 : Probe("block 0,3")
Cls : TileBlock img,5,0 : Probe("block 5,0")
Cls : TileBlock img,-3,-2 : Probe("block -3,-2")
Cls : TileImage img,2,5 : Probe("image 2,5")
Cls : MidHandle img : TileBlock img,0,0 : Probe("handle") : HandleImage img,0,0
Cls : Origin 3,2 : TileBlock img,0,0 : Origin 0,0 : Probe("origin 3,2")
Cls : Viewport 20,10,50,40 : TileBlock img,0,0 : Viewport 0,0,200,150 : Probe("viewport")
Print "aussen "+Hex(ReadPixel(5,5) And $FFFFFF)+" innen "+Hex(ReadPixel(25,15) And $FFFFFF)
Print "DONE"
End

Function Probe(name$)
	r$="" : g$=""
	For y=0 To 15
		If (ReadPixel(8,y) And $FFFFFF)=$FF0000 Then r$=r$+"1" Else r$=r$+"0"
	Next
	For x=0 To 31
		If (ReadPixel(x,4) And $FFFFFF)=$00FF00 Then g$=g$+"1" Else g$=g$+"0"
	Next
	Print name$+": rot "+r$+" gruen "+g$
End Function
