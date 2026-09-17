; 3D-16 - Sprites: Lage, Groesse, Handle, Drehung, View-Modi, Textur und Mischart.
;
; Gemessen am Original (2026-09-17). Kamera bei (0,0,-5), Zoom 1: bei z=0
; ist eine Einheit 32 Pixel, der Ursprung liegt bei (160,120). Gemessen wird
; die Ausdehnung der Farbe in der Mittelzeile/-spalte bzw. Pixel an Stellen.

Graphics3D 320,240,0,2
cam = CreateCamera()
CameraClsColor cam,40,40,40
PositionEntity cam,0,0,-5

Function PxR$(x1,y1,x2,y2)
	RenderWorld
	Return Px(x1,y1) + " " + Px(x2,y2)
End Function

Function Px$(x,y)
	Return Hex(ReadPixel(x,y) And $FFFFFF)
End Function

; Ausdehnung einer Nicht-Hintergrund-Farbe: links,rechts in Zeile y; oben,unten in Spalte x
Function Aus$(x,y)
	LockBuffer BackBuffer()
	l=-1 : r=-1 : o=-1 : u=-1
	For i=0 To 319
		If (ReadPixelFast(i,y) And $FFFFFF)<>$282828
			If l<0 Then l=i
			r=i
		EndIf
	Next
	For i=0 To 239
		If (ReadPixelFast(x,i) And $FFFFFF)<>$282828
			If o<0 Then o=i
			u=i
		EndIf
	Next
	UnlockBuffer BackBuffer()
	Return l+","+r+" / "+o+","+u
End Function

Function Bild$(x,y)
	RenderWorld
	Return Aus(x,y) + " tris " + TrisRendered()
End Function

s = CreateSprite()
EntityColor s,255,0,0
Print "1 klasse " + EntityClass(s)
Print "2 vorgabe: " + Bild(160,120) + " farbe " + Px(160,120)
ScaleSprite s,2,1
Print "3 scale 2,1: " + Bild(160,120)
ScaleSprite s,1,1
HandleSprite s,1,0
Print "4 handle 1,0: " + Bild(150,120)
HandleSprite s,0,0
ScaleSprite s,2,0.5
RotateSprite s,90
Print "5 scale 2,.5 rotate 90: " + Bild(160,120)
HandleSprite s,1,0
Print "6 dazu handle 1,0: " + Bild(160,100) + " / " + Aus(160,140)
HandleSprite s,0,0
RotateSprite s,0
ScaleSprite s,1,1
ScaleEntity s,2,2,2
Print "7 mode1 scaleentity 2: " + Bild(160,120)
SpriteViewMode s,2
Print "8 mode2 scaleentity 2: " + Bild(160,120)
ScaleEntity s,1,1,1
EntityAlpha s,1
TurnEntity s,0,180,0
Print "9 mode2 umgedreht: " + Bild(160,120)
RotateEntity s,0,0,0
RotateEntity s,0,0,30
SpriteViewMode s,1
Print "10 mode1 roll entity 30: " + Bild(160,120) + " ecke " + Px(185,100)
SpriteViewMode s,2
Print "11 mode2 roll entity 30: " + Bild(160,120) + " ecke " + Px(185,100)
RotateEntity s,0,0,0

; Kamera von oben schraeg
PositionEntity cam,0,4,-4
PointEntity cam,s
For m=1 To 4
	SpriteViewMode s,m
	Print "12 kamera schraeg mode " + m + ": " + Bild(160,120)
Next
; Kamera seitlich
PositionEntity cam,4,0,-4
PointEntity cam,s
For m=1 To 4
	SpriteViewMode s,m
	Print "13 kamera seitlich mode " + m + ": " + Bild(160,120)
Next
; Kamera seitlich und gerollt
RotateEntity cam,EntityPitch(cam),EntityYaw(cam),25
For m=1 To 4
	SpriteViewMode s,m
	Print "14 kamera gerollt mode " + m + ": " + Bild(160,120)
Next
FreeEntity s
PositionEntity cam,0,0,-5
RotateEntity cam,0,0,0

; Textur und Mischart
t = LoadSprite("tests/assets/quad.bmp")
Print "15 loadsprite quad: " + Bild(160,120) + " farben " + Px(140,100) + " " + Px(180,100) + " " + Px(140,140) + " " + Px(180,140)
FreeEntity t
t = LoadSprite("tests/assets/quad.bmp",2)
Print "16 loadsprite flag 2: " + PxR(140,100,180,140)
FreeEntity t
t = LoadSprite("tests/assets/maske.bmp",4)
Print "17 loadsprite flag 4: " + PxR(140,100,180,100)
FreeEntity t
t = LoadSprite("tests/assets/maske.bmp",1+4)
Print "18 loadsprite flag 5: " + PxR(140,100,180,100)
FreeEntity t
t = LoadSprite("tests/assets/gibtsnicht.bmp")
Print "19 fehlt: " + t

; CreateSprite mit Textur und Farbe
u = CreateSprite()
tex = LoadTexture("tests/assets/quad.bmp")
EntityTexture u,tex
Print "20 createsprite entitytexture: " + PxR(140,100,180,140)
EntityColor u,128,128,128
Print "21 dazu farbe 128: " + PxR(140,100,180,140)
EntityColor u,255,255,255
EntityBlend u,3
Print "22 blend 3: " + PxR(140,100,180,140)

; Kopie uebernimmt Groesse, Handle, Modus
ScaleSprite u,0.5,0.5
HandleSprite u,1,1
v = CopyEntity(u)
FreeEntity u
Print "23 kopie: " + Bild(150,130)

; Elternteil skaliert, Modus 2
FreeEntity v
p = CreatePivot()
ScaleEntity p,2,1,1
w = CreateSprite(p)
SpriteViewMode w,2
EntityColor w,0,0,255
Print "24 eltern skaliert mode2: " + Bild(160,120)
SpriteViewMode w,1
Print "25 eltern skaliert mode1: " + Bild(160,120)
End
