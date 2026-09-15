; BUG-63 - der Backbuffer enthaelt nach RenderWorld das 3D-Bild und die
; 2D-Zeichnungen darueber, und die Runtime zeichnet ueber mehrere Frames
; richtig, auch wenn 2D-Befehle, Freigeben und Wireframe sich abwechseln.
;
; Bis 2026-09-15 legte Graphics3D den 2D-Renderer mit dem Vorgabe-Backend an,
; unter Windows direct3d11 - ein eigener Puffer. Gemessen: keine 2D-Zeichnung
; war im 3D-Modus auf dem Bildschirm, LockBuffer/ReadPixel/GetColor sahen das
; 3D-Bild nicht (0,0,0), und ReadPixel ohne LockBuffer lieferte immer 0.
;
; Die erwarteten Werte sind am Original gemessen (Blitz3D, F:\dev\Blitz3D,
; 2026-09-15): dieselbe Logik schrieb dort in eine Datei, weil Print im
; Original nicht auf stdout geht. Rot ohne Licht ist 127,0,0 - das
; Umgebungslicht 127 (3D-10).

Graphics3D 320,240,32,2
cam = CreateCamera()
CameraClsColor cam,0,0,255

; 2D vor allem 3D-Aufbau: danach ist der Kontext des 2D-Renderers aktuell
Color 255,255,255
Rect 0,0,10,10

rot = CreateCube()
PositionEntity rot,0,0,5
EntityColor rot,255,0,0

links = CreateCube()
PositionEntity links,-3,0,6
tex = LoadTexture("tests/assets/test_rle8.bmp")
EntityTexture links,tex

For frame = 1 To 3
  If frame = 2
    ; Freigeben nach 2D-Zeichnen: GL-Aufrufe ausserhalb von RenderWorld
    Color 0,0,0
    Plot 1,1
    FreeEntity links
    FreeTexture tex
  EndIf
  If frame = 3 Then Wireframe False
  UpdateWorld
  RenderWorld
  Color 0,255,0
  Rect 20,20,10,10
  Color 255,255,0
  Text 250,200,"X"
  LockBuffer BackBuffer()
  Zeile "frame " + frame + " mitte " + Farbe(ReadPixelFast(160,120))
  Zeile "frame " + frame + " rand  " + Farbe(ReadPixelFast(300,120))
  Zeile "frame " + frame + " rect  " + Farbe(ReadPixelFast(25,25))
  Zeile "frame " + frame + " links gesetzt " + (Farbe(ReadPixelFast(62,120)) <> "0,0,255")
  UnlockBuffer BackBuffer()
  Zeile "frame " + frame + " readpixel mitte " + Farbe(ReadPixel(160,120))
  GetColor 300,120
  Zeile "frame " + frame + " getcolor rand " + ColorRed() + "," + ColorGreen() + "," + ColorBlue()
  Flip
Next
Ende()
End

Function Farbe$(p)
  Return ((p Shr 16) And 255) + "," + ((p Shr 8) And 255) + "," + (p And 255)
End Function
Function Zeile(s$)
  Print s
End Function
Function Ende()
End Function
