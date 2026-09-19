; BUG-166 - CreateCamera uebernimmt den aktuellen 2D-Viewport, ohne Viewport
; die volle Grafikgroesse; eine Kamera mit leerem Viewport (Breite oder Hoehe 0)
; loescht nicht und zeichnet nicht.
;
; Alle Werte am Original gemessen (2026-09-18/19). blox-n-balls blendet im
; Menue so seine Spielkamera aus (CameraViewport cam,0,0,0,0).

Graphics3D 640,480,0,2
SetBuffer BackBuffer()

Function Probe$(txt$)
	RenderWorld
	LockBuffer BackBuffer()
	a=ReadPixelFast(320,240) And $FFFFFF
	b=ReadPixelFast(5,5) And $FFFFFF
	c=ReadPixelFast(600,400) And $FFFFFF
	UnlockBuffer BackBuffer()
	Return txt+" mitte="+Hex(a)+" ecke="+Hex(b)+" unten="+Hex(c)+" tris="+TrisRendered()
End Function

; --- Teil 1: leere Viewports ---
cube=CreateCube()
PositionEntity cube,0,0,5
c1=CreateCamera() : CameraClsColor c1,255,0,0
c2=CreateCamera() : CameraClsColor c2,0,255,0
CameraViewport c2,0,0,0,0
Print Probe("A voll dann leer")
CameraViewport c2,100,100,0,50
Print Probe("A leer bei 100,100 h50")
FreeEntity c1 : FreeEntity c2
c2=CreateCamera() : CameraClsColor c2,0,255,0
CameraViewport c2,0,0,0,0
c1=CreateCamera() : CameraClsColor c1,255,0,0
Print Probe("B leer dann voll")
FreeEntity c1
Cls
Print Probe("C nur leer")
c1=CreateCamera() : CameraClsColor c1,255,0,0
EntityOrder c2,5
Print Probe("D leer zuerst per order")
EntityOrder c2,-5
Print Probe("E leer zuletzt per order")
CameraClsMode c2,0,0
Print Probe("F leer zuletzt ohne cls")
FreeEntity c1 : FreeEntity c2 : FreeEntity cube

; --- Teil 2: CreateCamera uebernimmt den 2D-Viewport ---
ClsColor 0,0,255
Cls
Viewport 100,100,200,100
c=CreateCamera()
CameraClsColor c,255,0,0
Viewport 0,0,640,480
RenderWorld
LockBuffer BackBuffer()
Print "innen " + Hex(ReadPixelFast(150,150) And $FFFFFF)
Print "aussen " + Hex(ReadPixelFast(50,50) And $FFFFFF)
Print "rechts " + Hex(ReadPixelFast(310,150) And $FFFFFF)
UnlockBuffer BackBuffer()
c2=CreateCamera()
CameraClsColor c2,0,255,0
Cls
RenderWorld
LockBuffer BackBuffer()
Print "zweite ohne viewport " + Hex(ReadPixelFast(50,50) And $FFFFFF)
UnlockBuffer BackBuffer()
