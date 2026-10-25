; BUG-119 - ImagesCollide und ImageRectCollide pruefen sichtbare Pixel.
;
; Wie gxCanvas::collide/rect_collide: im Ueberlappungsbereich muss ein Pixel
; in beiden Bildern eine andere Farbe als die Maskenfarbe haben; Frames und
; Griffpunkte zaehlen. Gemessen an Blitz3D 11.8 (build/bug119_20261025/coll),
; die Erwartung ist die Ausgabe des Originals. Bis dahin prueften beide nur
; die Rechtecke. Ausgelassen ist ein Fall nach MaskImage: das Original
; erneuert seine Bitmaske dabei nicht, wir pruefen gegen die aktuelle Maske.
Graphics 200,150,0,2
a=CreateImage(8,8)
SetBuffer ImageBuffer(a) : Cls : Color 255,0,0 : Plot 2,2
b=CreateImage(8,8)
SetBuffer ImageBuffer(b) : Cls : Color 0,255,0 : Plot 5,5
SetBuffer BackBuffer()
Print ImagesOverlap(a,0,0,b,0,0)
Print ImagesCollide(a,0,0,0,b,0,0,0)
Print ImagesCollide(a,0,0,0,b,-3,-3,0)
Print ImagesCollide(a,0,0,0,b,-2,-3,0)
Print ImagesCollide(a,3,3,0,b,0,0,0)
c=CreateImage(4,4)
Print ImagesCollide(c,0,0,0,c,0,0,0)
d=CreateImage(4,4,2)
SetBuffer ImageBuffer(d,1) : Cls : Color 255,255,255 : Plot 0,0
SetBuffer BackBuffer()
Print ImagesCollide(d,0,0,0,a,-2,-2,0)
Print ImagesCollide(d,0,0,1,a,-2,-2,0)
MidHandle a
Print ImagesCollide(a,4,4,0,d,0,0,1)
Print ImagesCollide(a,2,2,0,d,0,0,1)
HandleImage a,0,0
MaskImage a,255,0,0
Print ImagesCollide(a,0,0,0,c,0,0,0)
MaskImage a,0,0,0
Print ImageRectCollide(a,0,0,0,0,0,2,2)
Print ImageRectCollide(a,0,0,0,2,2,1,1)
Print ImageRectCollide(a,10,10,0,12,12,1,1)
Print ImageRectOverlap(a,0,0,0,0,2,2)
Print ImageRectCollide(d,0,0,1,0,0,1,1)
Print ImageRectCollide(d,0,0,0,0,0,1,1)
