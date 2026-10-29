; BUG-178 - Flip wartet auch im Fenster auf den Bildaufbau, Flip 0 nie.
;
; Flip hat in Blitz3D das Argument vwait mit der Vorgabe True: "Flip" und
; "Flip 1" warten, "Flip 0" muss man ausdruecklich schreiben. Das heutige
; Original haelt das im Fenster nicht mehr ein (gemessen am 2026-09-22 auf
; einem 60-Hz-Monitor, build/flip_20260922: 164 fps im 2D- und 149 fps im
; 3D-Fenster mit Flip 1), weil sein DirectDraw-Blit unter DWM nicht
; synchronisiert. Programme ohne eigene Zeitbremse sind aber fuer die
; Bildwiederholrate geschrieben und waeren ohne Warten unspielbar schnell
; (18 von 76 Programmen in Pakz001/Blitz3d). Entscheidung des Nutzers am
; 2026-10-29: Flip und Flip 1 warten wie im Vollbild, Flip 0 bleibt frei.
;
; Geprueft wird je im 2D- und im 3D-Fenster, fuer Monitore bis 250 Hz:
;   - Flip 0 bleibt unter der halben Zeit, die 250 Hz erzwingen wuerden,
;   - Flip 1 und Flip ohne Argument brauchen mindestens diese Zeit.
; GraphicsRate() ist im Fenster 0 und taugt nicht als Massstab.
;
; Das Vollbild ist nicht Teil des Tests, weil es den Bildschirm waehrend der
; Suite umschalten wuerde - es wurde von Hand gegen das Original gemessen.

n = 60

Graphics 320,240,0,2
SetBuffer BackBuffer()
g = Grenze(n)
For i = 1 To 30 : Cls : Flip 0 : Next
t = MilliSecs() : For i = 1 To n : Cls : Flip 0 : Next : frei = MilliSecs() - t
t = MilliSecs() : For i = 1 To n : Cls : Flip 1 : Next : eins = MilliSecs() - t
t = MilliSecs() : For i = 1 To n : Cls : Flip   : Next : vorg = MilliSecs() - t
Print "2D Fenster Flip 0 ohne Warten: " + Frei(frei, g)
Print "2D Fenster Flip 1 wartet: " + Wartet(eins, g)
Print "2D Fenster Flip wartet: " + Wartet(vorg, g)
EndGraphics

Graphics3D 320,240,0,2
SetBuffer BackBuffer()
g = Grenze(n)
cam = CreateCamera()
cube = CreateCube()
PositionEntity cube,0,0,5
For i = 1 To 30 : RenderWorld : Flip 0 : Next
t = MilliSecs() : For i = 1 To n : RenderWorld : Flip 0 : Next : frei = MilliSecs() - t
t = MilliSecs() : For i = 1 To n : RenderWorld : Flip 1 : Next : eins = MilliSecs() - t
t = MilliSecs() : For i = 1 To n : RenderWorld : Flip   : Next : vorg = MilliSecs() - t
Print "3D Fenster Flip 0 ohne Warten: " + Frei(frei, g)
Print "3D Fenster Flip 1 wartet: " + Wartet(eins, g)
Print "3D Fenster Flip wartet: " + Wartet(vorg, g)

; So lange brauchen n Bilder mindestens, wenn Flip auf einen Monitor mit
; hoechstens 250 Hz wartet: 60 Bilder 240 ms (auf 60 Hz sind es 1000 ms).
Function Grenze(n)
	Return (n * 1000) / 250
End Function

; Ohne Warten brauchen 60 Bilder gut 10 ms; die Haelfte der Grenze laesst
; Schwankungen der Maschine durchgehen und trennt beides trotzdem sicher.
Function Frei$(ms, grenze)
	If ms * 2 < grenze Then Return "ja"
	Return "nein"
End Function

Function Wartet$(ms, grenze)
	If ms >= grenze Then Return "ja"
	Return "nein"
End Function
