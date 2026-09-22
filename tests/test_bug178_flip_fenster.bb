; BUG-178 - Flip wartet nur im Vollbild auf den Bildaufbau, im Fenster nie.
;
; Im Original laeuft ein Fensterprogramm weit ueber der Bildwiederholrate:
; gemessen am 2026-09-22 auf einem 60-Hz-Monitor (Programme und Rohwerte in
; build/flip_20260922, 300 Bilder je Phase) 164 fps im 2D-Fenster und 149 fps
; im 3D-Fenster, jeweils mit Flip 1. Im Vollbild wartet das Original dagegen
; genau auf den Bildaufbau (60,6 fps) und mit Flip 0 nicht (2679 fps).
; Blitz3Ds DirectDraw-Blit im Fenster synchronisiert unter dem heutigen
; Windows/DWM nicht; Programme timen ihre Bildrate selbst.
;
; Geprueft wird zweierlei, je im 2D- und im 3D-Fenster:
;   - das Argument macht keinen Unterschied (Flip 1 dauert nicht laenger als
;     Flip 0) - das ist die Eigenschaft, um die es geht,
;   - die Schleife bleibt unter der Zeit, die die Bildwiederholrate erzwingen
;     wuerde - das ist das Symptom aus dem Fehlerbericht.
;
; Das Vollbild ist nicht Teil des Tests, weil es den Bildschirm waehrend der
; Suite umschalten wuerde - es wurde von Hand gegen das Original gemessen.

n = 120

Graphics 320,240,0,2
SetBuffer BackBuffer()
g = Grenze(n)
For i = 1 To 60 : Cls : Flip 0 : Next
t = MilliSecs() : For i = 1 To n : Cls : Flip 0 : Next : frei = MilliSecs() - t
t = MilliSecs() : For i = 1 To n : Cls : Flip 1 : Next : eins = MilliSecs() - t
t = MilliSecs() : For i = 1 To n : Cls : Flip   : Next : vorg = MilliSecs() - t
Print "2D Fenster Flip 1 wie Flip 0: " + Gleich(eins, frei)
Print "2D Fenster Flip wie Flip 0: " + Gleich(vorg, frei)
Print "2D Fenster Flip 1 ohne Warten: " + Frei(eins, g)
Print "2D Fenster Flip ohne Warten: " + Frei(vorg, g)
EndGraphics

Graphics3D 320,240,0,2
SetBuffer BackBuffer()
g = Grenze(n)
cam = CreateCamera()
cube = CreateCube()
PositionEntity cube,0,0,5
For i = 1 To 60 : RenderWorld : Flip 0 : Next
t = MilliSecs() : For i = 1 To n : RenderWorld : Flip 0 : Next : frei = MilliSecs() - t
t = MilliSecs() : For i = 1 To n : RenderWorld : Flip 1 : Next : eins = MilliSecs() - t
t = MilliSecs() : For i = 1 To n : RenderWorld : Flip   : Next : vorg = MilliSecs() - t
Print "3D Fenster Flip 1 wie Flip 0: " + Gleich(eins, frei)
Print "3D Fenster Flip wie Flip 0: " + Gleich(vorg, frei)
Print "3D Fenster Flip 1 ohne Warten: " + Frei(eins, g)
Print "3D Fenster Flip ohne Warten: " + Frei(vorg, g)

; So lange braeuchten n Bilder, wenn Flip auf den Monitor wartete.
Function Grenze(n)
	r = GraphicsRate()
	If r <= 0 Then r = 60
	Return (n * 1000) / r
End Function

; Wartet Flip auf den Bildaufbau, dauert die Schleife ein Vielfaches: bei 120
; Bildern auf einem 60-Hz-Monitor 2000 ms gegenueber gut 20 ms. Der Spielraum
; laesst Schwankungen der Maschine durchgehen und faellt trotzdem auf.
Function Gleich$(ms, frei)
	If ms <= frei * 3 + 300 Then Return "ja"
	Return "nein"
End Function

Function Frei$(ms, grenze)
	If ms * 2 < grenze Then Return "ja"
	Return "nein"
End Function
