; BUG-129 - mehrere Kameras: Viewports, Loeschen und Reihenfolge.
;
; Alle Werte am Original gemessen (2026-09-17):
;  - jede Kamera loescht nur ihren Viewport, ausserhalb bleibt stehen, was
;    vorher im Backbuffer war; Geometrie wird am Viewport abgeschnitten;
;  - gerendert wird in der Reihenfolge von World::render: Kameras in
;    Baumreihenfolge (Wurzeln in Einfuegereihenfolge, Kinder nach ihrem
;    Elternteil) in eine priority_queue nach EntityOrder, hoehere Order zuerst.
;    Bei gleicher Order entscheidet die Heap-Mechanik des Originals - fuenf
;    gleiche Kameras laufen 0,2,4,1,3.
; Die Reihenfolge wird paarweise bestimmt: nur zwei Kameras loeschen, die
; zuletzt gerenderte gewinnt. "rang" ist je Kamera die Zahl der Kameras, die
; vor ihr rendern.

Graphics3D 320,240,0,2
SetBuffer BackBuffer()
Dim cams(10)
Dim wins(10)

Function P$(x,y)
	Return ReadPixel(x,y) And $FFFFFF
End Function

Function Rang$(n)
	For k = 0 To n-1 : wins(k) = 0 : Next
	For i = 0 To n-1
		For j = i+1 To n-1
			For k = 0 To n-1 : CameraClsMode cams(k), 0, 0 : Next
			CameraClsMode cams(i), 1, 1
			CameraClsMode cams(j), 1, 1
			RenderWorld
			w = (ReadPixel(160,120) And $FF) - 1
			wins(w) = wins(w) + 1
		Next
	Next
	s$ = "rang:"
	For k = 0 To n-1 : s = s + " " + wins(k) : Next
	Return s
End Function

; --- Viewports, Loeschen, EntityOrder

c = CreateCube() : EntityFX c,1 : EntityColor c,255,128,0

; 1) BUG-129: zwei Kameras, zweiter Viewport rechts unten
cam = CreateCamera() : PositionEntity cam,0,0,-5 : CameraViewport cam,0,0,160,120 : CameraClsColor cam,0,0,0
cam2 = CreateCamera() : PositionEntity cam2,0,0,-5 : CameraViewport cam2,160,120,160,120 : CameraClsColor cam2,0,0,0
ClsColor 0,0,255 : Cls
RenderWorld
Print "1 vp1 mitte " + P(80,60) + " vp2 mitte " + P(240,180) + " ausserhalb " + P(240,60) + " " + P(80,180)

; 2) drei Kameras, eigene Cls-Farben, Rand eines Viewports
cam3 = CreateCamera() : PositionEntity cam3,0,0,-5 : CameraViewport cam3,160,0,160,120 : CameraClsColor cam3,0,255,0
CameraClsColor cam,255,0,0
ClsColor 255,255,255 : Cls
RenderWorld
Print "2 vp1 ecke " + P(2,2) + " vp3 ecke " + P(318,2) + " vp2 ecke " + P(318,238) + " frei " + P(2,238) + " mitte3 " + P(240,60)
; Geometrie wird am Viewport abgeschnitten: grosser Wuerfel in vp3
ScaleEntity c,10,10,10
RenderWorld
Print "2b grosser wuerfel, frei " + P(2,238) + " vp3 " + P(318,2)
ScaleEntity c,1,1,1
FreeEntity cam3

; 3) Ueberlappung bei gleicher Order: cam voll rot, cam2 klein blau in der Mitte
CameraViewport cam,0,0,320,240 : CameraClsColor cam,255,0,0
CameraViewport cam2,120,80,80,80 : CameraClsColor cam2,0,0,255
HideEntity c
RenderWorld
Print "3 mitte (cam zuerst erzeugt) " + P(160,120)
; neu erzeugt in umgekehrter Reihenfolge
FreeEntity cam : FreeEntity cam2
camB = CreateCamera() : CameraViewport camB,120,80,80,80 : CameraClsColor camB,0,0,255
camA = CreateCamera() : CameraViewport camA,0,0,320,240 : CameraClsColor camA,255,0,0
RenderWorld
Print "3b mitte (kleine zuerst erzeugt) " + P(160,120)

; 4) EntityOrder: hoehere Order zuerst oder zuletzt?
EntityOrder camB,1
RenderWorld
Print "4a kleine order 1 " + P(160,120)
EntityOrder camB,-1
RenderWorld
Print "4b kleine order -1 " + P(160,120)
EntityOrder camB,0
EntityOrder camA,1
RenderWorld
Print "4c grosse order 1 " + P(160,120)

; 5) CameraClsMode 0,0: Viewport loescht nicht
EntityOrder camA,0
FreeEntity camA
ClsColor 0,255,0 : Cls
CameraClsMode camB,0,1
RenderWorld
Print "5 clsmode 0 " + P(160,120)
FreeEntity camB
FreeEntity c

; --- gleiche Order, 2 bis 10 Kameras
For n = 2 To 10
	For k = 0 To n-1
		cams(k) = CreateCamera()
		CameraClsColor cams(k), 0, 0, k+1
	Next
	Print "n" + n + " " + Rang(n)
	For k = 0 To n-1 : FreeEntity cams(k) : Next
Next

; --- gemischte EntityOrder
For t = 1 To 12
	Read n
	For k = 0 To n-1
		cams(k) = CreateCamera()
		CameraClsColor cams(k), 0, 0, k+1
		Read o
		EntityOrder cams(k), o
	Next
	Print "gemischt " + t + " " + Rang(n)
	For k = 0 To n-1 : FreeEntity cams(k) : Next
Next

; --- Hierarchie und EntityParent
p1 = CreatePivot()
cams(0) = CreateCamera()
cams(1) = CreateCamera(p1)
cams(2) = CreateCamera()
cams(3) = CreateCamera(p1)
p2 = CreatePivot()
cams(4) = CreateCamera(p2)
cams(5) = CreateCamera(cams(1))
For k = 0 To 5 : CameraClsColor cams(k), 0, 0, k+1 : Next
Print "hierarchie " + Rang(6)
EntityParent cams(0), p2
Print "cam0 unter p2 " + Rang(6)
EntityParent cams(1), p1
Print "cam1 gleicher parent " + Rang(6)
EntityParent cams(3), 0
Print "cam3 zur wurzel " + Rang(6)
EntityOrder cams(2), 1 : EntityOrder cams(5), 1
Print "order 2 und 5 " + Rang(6)

Print "fertig"
End

Data 8,1,0,0,0,0,2,0,-1
Data 4,0,1,1,0
Data 8,0,1,0,2,-1,0,0,0
Data 9,2,2,0,0,2,-1,-1,0,1
Data 4,2,1,2,0
Data 9,1,2,0,-1,1,0,2,-1,0
Data 4,1,0,2,-1
Data 7,0,-1,2,0,0,2,0
Data 4,0,-1,0,0
Data 9,2,2,2,2,0,0,0,0,0
Data 5,0,-1,2,2,0
Data 6,-1,-1,0,-1,1,0
