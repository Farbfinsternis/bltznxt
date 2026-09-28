; CreatePlane - die unendliche Ebene (blitz3d/planemodel.cpp).
;
; Eine Ebene ist y = 0 im eigenen Raum, Normale +y. Gezeichnet wird je
; Kamera der Teil, den der Sichtkegel sieht, in segs x segs Feldern; von
; unten ist sie unsichtbar. Texturkoordinaten sind x und z der Ebene.
; Kollision und Picking (Methode 2) treffen die Ebene selbst, um den Radius
; angehoben. Am Original gemessen (2026-09-28): dieselbe Datei mit
; WriteLine statt Print ergibt Zeile fuer Zeile diese Ausgabe.

Graphics3D 320,240,0,2

Function C$(x#)
	Return Int(Floor(x * 1000.0 + 0.5))
End Function

Function Pixel$(x, y)
	LockBuffer BackBuffer()
	p = ReadPixelFast(x, y, BackBuffer())
	UnlockBuffer BackBuffer()
	Return ((p Shr 16) And 255) + "," + ((p Shr 8) And 255) + "," + (p And 255)
End Function

; 1) Klasse, Kopie
pl = CreatePlane()
kopie = CopyEntity(pl)
Print "1 klasse " + EntityClass(pl) + " kopie " + EntityClass(kopie)
FreeEntity kopie
FreeEntity pl

; 2) Eine Kugel faellt auf die Ebene: sie bleibt um den Radius darueber
pl = CreatePlane()
EntityType pl, 2
k = CreatePivot()
EntityRadius k, 0.5
EntityType k, 1
Collisions 1, 2, 2, 2
PositionEntity k, 0, 3, 0
ResetEntity k
For i = 1 To 5
	TranslateEntity k, 0.3, -1, 0
	UpdateWorld
Next
Print "2 fallen: y " + C(EntityY(k)) + " x " + C(EntityX(k)) + " kollisionen " + CountCollisions(k)
If CountCollisions(k)
	Print "2 treffer: y " + C(CollisionY(k, 1)) + " n " + C(CollisionNX(k, 1)) + "," + C(CollisionNY(k, 1)) + "," + C(CollisionNZ(k, 1)) + " mit ebene " + (CollisionEntity(k, 1) = pl) + " dreieck " + CollisionTriangle(k, 1)
EndIf
; von unten kommend: keine Kollision, sie faellt hindurch
PositionEntity k, 0, -3, 0
ResetEntity k
TranslateEntity k, 0, 4, 0
UpdateWorld
Print "2 von unten: y " + C(EntityY(k)) + " kollisionen " + CountCollisions(k)
FreeEntity k

; 3) Picking: nur mit EntityPickMode 2
e = LinePick(0.5, 10, 0.25, 0, -20, 0)
Print "3 ohne pickmode: " + (e <> 0)
EntityPickMode pl, 2
e = LinePick(0.5, 10, 0.25, 0, -20, 0)
Print "3 senkrecht: " + (e = pl) + " " + C(PickedX()) + "," + C(PickedY()) + "," + C(PickedZ()) + " n " + C(PickedNX()) + "," + C(PickedNY()) + "," + C(PickedNZ())
e = LinePick(0.5, -10, 0.25, 0, 20, 0)
Print "3 von unten: " + (e <> 0)
e = LinePick(0, 10, 0, 0, -5, 0)
Print "3 zu kurz: " + (e <> 0)
; gekippt und verschoben
PositionEntity pl, 0, -2, 0
RotateEntity pl, 0, 0, 30
e = LinePick(1, 10, 0, 0, -20, 0)
Print "3 gekippt: " + (e = pl) + " " + C(PickedX()) + "," + C(PickedY()) + "," + C(PickedZ()) + " n " + C(PickedNX()) + "," + C(PickedNY()) + "," + C(PickedNZ())
; gestreckt: die Normale bleibt senkrecht auf der Ebene
ScaleEntity pl, 1, 1, 3
RotateEntity pl, 30, 0, 0
e = LinePick(0, 10, 1, 0, -20, 0)
Print "3 gestreckt: " + (e = pl) + " " + C(PickedY()) + " n " + C(PickedNX()) + "," + C(PickedNY()) + "," + C(PickedNZ())
; mit Radius
ScaleEntity pl, 1, 1, 1
RotateEntity pl, 0, 0, 0
e = LinePick(0, 10, 0, 0, -20, 0, 0.5)
Print "3 radius: " + (e = pl) + " " + C(PickedY())
FreeEntity pl

; 4) Zeichnen: Farbe, von unten unsichtbar, Felder
cam = CreateCamera()
CameraClsColor cam, 0, 0, 80
pl = CreatePlane()
EntityColor pl, 255, 0, 0
EntityFX pl, 1
PositionEntity cam, 0, 2, 0
RotateEntity cam, 90, 0, 0
RenderWorld
Print "4 von oben: " + Pixel(160, 120) + " dreiecke " + TrisRendered()
PositionEntity cam, 0, -2, 0
RotateEntity cam, -90, 0, 0
RenderWorld
Print "4 von unten: " + Pixel(160, 120) + " dreiecke " + TrisRendered()
; zum Horizont: oben Himmel, unten Ebene
PositionEntity cam, 0, 2, 0
RotateEntity cam, 10, 0, 0
RenderWorld
Print "4 horizont: oben " + Pixel(160, 10) + " unten " + Pixel(160, 230) + " dreiecke " + TrisRendered()
FreeEntity pl
pl = CreatePlane(4)
EntityColor pl, 255, 0, 0
EntityFX pl, 1
RenderWorld
Print "4 vier felder: dreiecke " + TrisRendered()
FreeEntity pl

; 5) Textur: je Einheit einmal, Schachbrett aus vier Feldern
tex = CreateTexture(2, 2)
SetBuffer TextureBuffer(tex)
WritePixel 0, 0, $FFFF0000 : WritePixel 1, 1, $FFFF0000
WritePixel 1, 0, $FF00FF00 : WritePixel 0, 1, $FF00FF00
SetBuffer BackBuffer()
pl = CreatePlane()
EntityTexture pl, tex
EntityFX pl, 1
PositionEntity cam, 0.25, 2, 0.25
RotateEntity cam, 90, 0, 0
RenderWorld
Print "5 textur: " + Pixel(160, 120)
PositionEntity cam, 0.75, 2, 0.25
RenderWorld
Print "5 textur daneben: " + Pixel(160, 120)
PositionEntity cam, 1.25, 2, 0.25
RenderWorld
Print "5 textur naechste einheit: " + Pixel(160, 120)
ScaleEntity pl, 2, 1, 2
PositionEntity cam, 0.5, 2, 1.5
RenderWorld
Print "5 textur gestreckt: " + Pixel(160, 120)
End
