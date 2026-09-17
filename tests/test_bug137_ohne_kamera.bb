; BUG-137 - RenderWorld ohne aktive Kamera zeichnet nichts, auch keinen
; Hintergrund.
;
; Gemessen am Original (2026-09-17): ohne jede Kamera, mit versteckter Kamera
; und mit CameraProjMode 0 bleibt im Backbuffer stehen, was vorher dort war -
; das Cls, gezeichnetes 2D oder das letzte gerenderte Bild. TrisRendered ist 0.
; Bis dahin loeschte ein erfundener Ersatzzustand das Bild, und eine Kamera mit
; CameraProjMode 0 renderte perspektivisch weiter.

Graphics3D 320,240,0,2
SetBuffer BackBuffer()

; A) nie eine Kamera: RenderWorld nach Cls in Rot
ClsColor 255,0,0 : Cls
c = CreateCube() : EntityFX c,1 : EntityColor c,0,255,0
RenderWorld
Print "A1 ohne kamera nach Cls rot: " + (ReadPixel(160,120) And $FFFFFF)
Print "A2 TrisRendered: " + TrisRendered()

; B) Kamera rendert, dann versteckt
cam = CreateCamera()
CameraClsColor cam,0,0,0
PositionEntity cam,0,0,-5
RenderWorld
Print "B1 mit kamera: " + (ReadPixel(160,120) And $FFFFFF) + " tris " + TrisRendered()
HideEntity cam
RenderWorld
Print "B2 kamera versteckt: " + (ReadPixel(160,120) And $FFFFFF) + " tris " + TrisRendered()

; C) Text vor RenderWorld ohne Kamera bleibt stehen
ClsColor 0,0,255 : Cls
Color 255,255,255 : Rect 0,0,10,10,1
RenderWorld
Print "C1 hintergrund: " + (ReadPixel(160,120) And $FFFFFF) + " rechteck: " + (ReadPixel(5,5) And $FFFFFF)

; D) Kamera mit CameraProjMode 0 (aus)
ShowEntity cam
CameraProjMode cam,0
ClsColor 255,255,0 : Cls
RenderWorld
Print "D1 projmode 0: " + (ReadPixel(160,120) And $FFFFFF) + " tris " + TrisRendered()
End
