; BUG-195 (Gruppe 3) - TFormImage-Familie mit mehreren Frames, RectsOverlap.
;
; CreateImage(4,2,3) : ScaleImage stuerzte ab, weil nie bemalte Frames keine
; Pixelkopie hatten. Alle Werte am Original gemessen (2026-10-04).

Graphics 320,240,0,2
; 1) Absturzfall: 3 Frames, nie gezeichnet
i = CreateImage(4,2,3)
ScaleImage i,3,2
Print ImageWidth(i) + "x" + ImageHeight(i) + " h=" + ImageXHandle(i) + "," + ImageYHandle(i)
; 2) Frame 1 bemalt, dann skaliert
j = CreateImage(4,2,3)
SetBuffer ImageBuffer(j,1)
Color 255,0,0 : Rect 0,0,4,2,1
SetBuffer BackBuffer()
ScaleImage j,2,2
Print ImageWidth(j) + "x" + ImageHeight(j)
SetBuffer ImageBuffer(j,0) : Print Hex(ReadPixel(3,1))
SetBuffer ImageBuffer(j,1) : Print Hex(ReadPixel(3,1))
SetBuffer ImageBuffer(j,2) : Print Hex(ReadPixel(3,1))
SetBuffer BackBuffer()
; 3) Rotation mehrerer Frames, MidHandle
k = CreateImage(8,4,2)
MidHandle k
RotateImage k,90
Print ImageWidth(k) + "x" + ImageHeight(k) + " h=" + ImageXHandle(k) + "," + ImageYHandle(k)
; 4) ResizeImage mehrerer Frames
r = CreateImage(5,3,2)
ResizeImage r,10,9
Print ImageWidth(r) + "x" + ImageHeight(r)
; 5) Zeichnen eines transformierten Frames
DrawImage j,10,10,2
DrawImage k,50,50,1
; 6) RectsOverlap
Print RectsOverlap(0,0,10,10,20,20,5,5) + "" + RectsOverlap(0,0,10,10,5,5,10,10) + RectsOverlap(0,0,10,10,9,9,1,1) + RectsOverlap(0,0,10,10,10,0,5,5)
Print "ok"
