; BLTZCRFT - Abbauen ueber die Zeit und die zur Laufzeit gemalten Risse.
;
; Halten in Schritten von 0.1 s: Erde (0.6 s) bricht im sechsten Schritt,
; Stein (2.0 s) im zwanzigsten; ein anderer Block faengt von vorn an,
; Grundgestein bricht nie (und zeigt keine Risse), nach einem Bruch kommt
; eine Pause von 0.2 s, in der der naechste Block noch nicht anfaengt.
;
; Die Risstexturen sind maskiert (Flag 4): wo kein Riss ist, steht Alpha 0.
; Vor einem roten Wuerfel muss dort Rot durchscheinen, auf einem Riss nicht
; (BUG-192: bis dahin pruefte BLTZNXT die Farbe statt des Alphakanals).

Include "../samples/bltzcrft/bloecke.bb"
Include "../samples/bltzcrft/abbau.bb"

Graphics3D 320, 240, 0, 2
SetBuffer BackBuffer()
Bloecke_Arten()

Function Halten$(x, b, schritte)
	s$ = ""
	For i = 1 To schritte
		If Abbau_Halten(x, 0, 0, b, 0.1) Then s = s + "|" + i + " " Else s = s + Abbau_Stufe() + " "
	Next
	Return s
End Function

Print "erde   " + Halten(1, B_ERDE, 7)
Abbau_Loslassen()
Print "stein  " + Halten(2, B_STEIN, 21)
Abbau_Loslassen()
Halten(3, B_STEIN, 10)
Print "anderer block faengt neu an: " + Halten(4, B_STEIN, 1)
Abbau_Loslassen()
Print "loslassen setzt zurueck, stufe " + Abbau_Stufe()
Print "grund  " + Halten(5, B_GRUND, 30)
Abbau_Loslassen()
Print "laub bricht: " + Halten(6, B_LAUB, 3)
Print "pause nach dem bruch: " + Halten(7, B_LAUB, 6)

; Risse
Abbau_Laden()
Function Deckend(tex)
	n = 0
	LockBuffer TextureBuffer(tex)
	For y = 0 To 15 : For x = 0 To 15
		If (ReadPixelFast(x * TEX_ZOOM + 3, y * TEX_ZOOM + 3, TextureBuffer(tex)) Shr 24) And 255 Then n = n + 1
	Next : Next
	UnlockBuffer TextureBuffer(tex)
	Return n
End Function
s$ = ""
steigt = True : vorher = 0
For st = 1 To AB_STUFEN
	d = Deckend(ab_tex(st))
	If d < vorher Then steigt = False
	vorher = d
	If st = 1 Or st = AB_STUFEN Then s = s + "stufe " + st + ": " + d + " texel  "
Next
Print s + "steigt " + steigt

; gezeichnet vor einem roten Wuerfel
cam = CreateCamera() : CameraClsColor cam, 0, 0, 255
rot = CreateCube() : EntityFX rot, 1 : EntityColor rot, 255, 0, 0 : ScaleEntity rot, 0.5, 0.5, 0.5
PositionEntity rot, 0.5, 0.5, 0.5
PositionEntity cam, 0.5, 0.5, -1.2
ab_x = 0 : ab_y = 0 : ab_z = 0 : ab_aktiv = True : ab_anteil = 0.95
Abbau_Zeigen()
RenderWorld
rot_n = 0 : dunkel_n = 0
LockBuffer BackBuffer()
For y = 60 To 180 Step 4
	For x = 100 To 220 Step 4
		c = ReadPixelFast(x, y) And $FFFFFF
		If c = $FF0000 Then rot_n = rot_n + 1
		If ((c Shr 16) And 255) < 80 And (c And 255) < 80 Then dunkel_n = dunkel_n + 1
	Next
Next
UnlockBuffer BackBuffer()
Print "gezeichnet: rot scheint durch " + (rot_n > 200) + ", risse sichtbar " + (dunkel_n > 20)
ab_aktiv = False
Abbau_Zeigen()
RenderWorld
rot_n = 0
LockBuffer BackBuffer()
For y = 60 To 180 Step 4
	For x = 100 To 220 Step 4
		If (ReadPixelFast(x, y) And $FFFFFF) = $FF0000 Then rot_n = rot_n + 1
	Next
Next
UnlockBuffer BackBuffer()
Print "ohne abbau keine risse: " + (rot_n = 31 * 31)
End
