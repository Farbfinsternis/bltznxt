; BUG-169 - durchscheinende Flaechen pruefen den Z-Puffer, schreiben aber
; nicht hinein (im Original ZMODE_CMPONLY im durchsichtigen Durchgang).
;
; Nachbau des Menues von blox-n-balls: ein kleiner Funke und ein grosses
; Sprite mit Alpha 0.05 in genau derselben Tiefe an einem skalierten Pivot.
; Bei uns verschwand der Funke, weil das fast unsichtbare Sprite zuerst
; gezeichnet wurde und in den Z-Puffer schrieb. Werte am Original gemessen
; (2026-09-19).

Graphics3D 400,300,0,2
SetBuffer BackBuffer()
cam=CreateCamera()
PositionEntity cam,0,0,0.5
CameraClsColor cam,0,0,0
CameraRange cam,.1,1000
overlay=CreatePivot()
PositionEntity overlay,-1,0.75,1.5
ScaleEntity overlay,0.005,-0.005,-0.005

Function Messe$(txt$)
	RenderWorld
	LockBuffer BackBuffer()
	r=0 : n=0
	For y=140 To 160
		For x=190 To 210
			p=ReadPixelFast(x,y)
			r=r+((p Shr 16) And 255)
			If ((p Shr 16) And 255)>100 Then n=n+1
		Next
	Next
	UnlockBuffer BackBuffer()
	Return txt+" rot="+r+" hell="+n
End Function

funke=CreateSprite()
EntityColor funke,255,0,0
ScaleSprite funke,.02,.02
SpriteViewMode funke,2
EntityParent funke,overlay
PositionEntity funke,200,150,1
Print Messe("nur funke")

gross=CreateSprite()
EntityColor gross,0,0,255
ScaleSprite gross,1,1
SpriteViewMode gross,2
EntityAlpha gross,0.05
EntityParent gross,overlay
PositionEntity gross,200,150,1
Print Messe("grosses alpha-sprite gleiche tiefe")
PositionEntity gross,200,150,5
Print Messe("davor")
PositionEntity gross,200,150,0.5
Print Messe("dahinter")
EntityAlpha gross,1
PositionEntity gross,200,150,5
Print Messe("deckend davor")
PositionEntity gross,200,150,1
EntityAlpha gross,0.05
EntityAlpha funke,0.5
Print Messe("beide durchscheinend")
; Ein durchscheinender Wuerfel verdeckt nichts, was spaeter kommt
FreeEntity gross
EntityAlpha funke,1
box=CreateCube()
ScaleEntity box,0.3,0.3,0.01
PositionEntity box,0,0,1.3
EntityColor box,0,0,255
EntityAlpha box,0.05
Print Messe("alpha-wuerfel davor")
