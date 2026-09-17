; CreateMirror (3D-16): der Spiegel hat keine Geometrie. Fuer jeden Spiegel
; wird die Szene zuerst mit gespiegelter Kamera gezeichnet, danach die
; normale Szene darueber.
;
; Gemessen am Original (2026-09-17). Kamera bei (0,2,-8) blickt nach vorn;
; ein roter Wuerfel schwebt bei y=2, der Spiegel liegt in der Ebene y=0.

Graphics3D 320,240,0,2
cam = CreateCamera()
CameraClsColor cam,40,40,40
PositionEntity cam,0,2,-8

Function Px$(x,y)
	Return Hex(ReadPixel(x,y) And $FFFFFF)
End Function
Function Bild$()
	RenderWorld
	Return Px(160,60) + " " + Px(160,120) + " " + Px(160,180) + " tris " + TrisRendered()
End Function

wuerfel = CreateCube()
PositionEntity wuerfel,0,2,0
EntityColor wuerfel,255,0,0
EntityFX wuerfel,1
Print "1 ohne spiegel: " + Bild()

m = CreateMirror()
Print "2 klasse " + EntityClass(m) + " kinder " + CountChildren(m)
Print "3 mit spiegel: " + Bild()

; Spiegel angehoben
PositionEntity m,0,1,0
Print "4 spiegel bei y=1: " + Bild()
PositionEntity m,0,0,0

; Spiegel gedreht
RotateEntity m,90,0,0
Print "5 spiegel gekippt: " + Bild()
RotateEntity m,0,0,0

; Spiegel versteckt
HideEntity m
Print "6 versteckt: " + Bild()
ShowEntity m

; zweiter Spiegel
m2 = CreateMirror()
PositionEntity m2,0,4,0
Print "7 zwei spiegel: " + Bild()
FreeEntity m2

; Kopie eines Spiegels
k = CopyEntity(m)
Print "8 kopie klasse " + EntityClass(k)
FreeEntity k

; Spiegel als Kind eines Pivots
p = CreatePivot()
PositionEntity p,0,-1,0
EntityParent m,p
Print "9 spiegel als kind: " + Bild()
EntityParent m,0
PositionEntity m,0,0,0

; Wuerfel unter dem Spiegel
PositionEntity wuerfel,0,-2,0
Print "10 wuerfel unten: " + Bild()
PositionEntity wuerfel,0,2,0

; halbdurchsichtiger Boden ueber dem Spiegel
boden = CreateCube()
ScaleEntity boden,5,0.01,5
EntityColor boden,0,0,255
EntityFX boden,1
EntityAlpha boden,0.5
Print "11 mit boden: " + Bild()
FreeEntity boden
FreeEntity m
Print "12 spiegel weg: " + Bild()
End
