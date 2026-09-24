; BUG-168 - Sprites erscheinen im Spiegel.
;
; Die gespiegelte Kamera dreht die Achsen des Sprite-Quadrats mit um; das
; Original kehrt deshalb im gespiegelten Durchgang die Dreiecke um
; (Sprite::render, rc.isReflected()). Bei uns fiel das Quadrat dort unter
; die Rueckseitenpruefung, das Spiegelbild fehlte. Kamera schraeg von oben,
; Spiegel in der XZ-Ebene: das Sprite liegt in der oberen Bildhaelfte, sein
; Spiegelbild in der unteren. Werte am Original gemessen (2026-09-24).

Graphics3D 400,300,0,2
SetBuffer BackBuffer()
cam=CreateCamera()
CameraClsColor cam,0,0,80
PositionEntity cam,0,3,-4
RotateEntity cam,35,0,0

Function Messe$(txt$)
	RenderWorld
	LockBuffer BackBuffer()
	ob=0 : un=0
	For y=0 To 299
		For x=0 To 399
			If ((ReadPixelFast(x,y) Shr 16) And 255)>100 Then
				If y<150 Then ob=ob+1 Else un=un+1
			EndIf
		Next
	Next
	UnlockBuffer BackBuffer()
	Return txt+" oben="+ob+" unten="+un+" tris="+TrisRendered()
End Function

; Schraege Kanten rastern knapp anders als im Original (1 Pixel); hier zaehlt
; nur, ob Sprite und Spiegelbild da sind.
Function Grob$(txt$)
	RenderWorld
	LockBuffer BackBuffer()
	ob=0 : un=0
	For y=0 To 299
		For x=0 To 399
			If ((ReadPixelFast(x,y) Shr 16) And 255)>100 Then
				If y<150 Then ob=ob+1 Else un=un+1
			EndIf
		Next
	Next
	UnlockBuffer BackBuffer()
	Return txt+" oben="+(ob>1000)+" unten="+(un>200)
End Function

spr=CreateSprite()
EntityColor spr,255,0,0
ScaleSprite spr,0.5,0.5
PositionEntity spr,0,1,0
Print Messe("ohne spiegel")
m=CreateMirror()
Print Messe("mit spiegel")
For vm=2 To 3
	SpriteViewMode spr,vm
	RotateEntity spr,0,0,0
	Print Messe("modus "+vm)
	RotateEntity spr,0,180,0
	Print Messe("modus "+vm+" gedreht")
	RotateEntity spr,60,30,0
	RotateSprite spr,40
	Print Grob("modus "+vm+" schraeg")
	RotateSprite spr,0
Next
SpriteViewMode spr,1
RotateEntity spr,0,0,0
HideEntity m
Print Messe("spiegel versteckt")
End
