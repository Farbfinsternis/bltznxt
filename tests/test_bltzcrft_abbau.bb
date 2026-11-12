; BLTZCRFT - Abbauen ueber die Zeit und die zur Laufzeit gemalten Risse.
;
; Dauer wie in Minecraft mit der Hand: 1,5 x Haerte, wo die Hand genuegt,
; sonst 5 x Haerte. Halten in Schritten von 0.1 s: Erde (0.75 s) bricht im
; achten Schritt, Holz (3 s) im dreissigsten; Stein (7.5 s) in Schritten von
; 0.5 s im fuenfzehnten. Ein anderer Block faengt von vorn an, Grundgestein
; bricht nie (und zeigt keine Risse), nach einem Bruch kommt eine Pause von
; 0.25 s, in der der naechste Block noch nicht anfaengt. Was die Hand nicht
; abbauen kann, hinterlaesst nichts. Werkzeuge: passendes Werkzeug 4-mal so
; schnell, Ertrag bei Gestein erst mit Spitzhacke, Verschleiss je Block.
;
; Die Risstexturen sind maskiert (Flag 4): wo kein Riss ist, steht Alpha 0.
; Vor einem roten Wuerfel muss dort Rot durchscheinen, auf einem Riss nicht
; (BUG-192: bis dahin pruefte BLTZNXT die Farbe statt des Alphakanals).

Include "../samples/bltzcrft/bloecke.bb"
Include "../samples/bltzcrft/dinge.bb"
Include "../samples/bltzcrft/abbau.bb"
Include "../samples/bltzcrft/inventar.bb"

Graphics3D 320, 240, 0, 2
SetBuffer BackBuffer()
Bloecke_Arten()
Dinge_Arten()

Function Halten$(x, b, schritte, dt# = 0.1, d = 0, h = 0)
	s$ = ""
	For i = 1 To schritte
		If Abbau_Halten(x, 0, 0, b, dt, d, h) Then s = s + "|" + i + " " Else s = s + Abbau_Stufe() + " "
	Next
	Return s
End Function

Print "erde   " + Halten(1, B_ERDE, 9)
Abbau_Loslassen()
Print "holz   " + Halten(2, B_STAMM, 31)
Abbau_Loslassen()
Print "stein  " + Halten(2, B_STEIN, 16, 0.5)
Abbau_Loslassen()
Print "dauer kohle " + Abbau_Dauer(B_KOHLE) + ", sand " + Abbau_Dauer(B_SAND) + ", laub " + Abbau_Dauer(B_LAUB)
Print "ertrag erde " + dg_name(Block_Ertrag(B_ERDE)) + ", stamm " + dg_name(Block_Ertrag(B_STAMM)) + ", stein " + dg_name(Block_Ertrag(B_STEIN)) + ", kohle " + dg_name(Block_Ertrag(B_KOHLE)) + ", schnee " + dg_name(Block_Ertrag(B_SCHNEE))

; Werkzeuge: Tempo 4 auf passenden Bloecken, Ertrag bei Stein und Kohle erst
; mit der Spitzhacke, Eisenerz verlangt Stufe 2 - kaputt wirkt wie die Hand
H = STEIN_HALTBAR
Print "dauer stein hand " + Abbau_Dauer(B_STEIN) + ", hacke " + Abbau_Dauer(B_STEIN, G_STEINHACKE, H) + ", kaputte hacke " + Abbau_Dauer(B_STEIN, G_STEINHACKE, 0) + ", axt " + Abbau_Dauer(B_STEIN, G_STEINAXT, H)
Print "dauer stamm axt " + Abbau_Dauer(B_STAMM, G_STEINAXT, H) + ", erde schaufel " + Abbau_Dauer(B_ERDE, G_STEINSCHAUFEL, H) + ", eisen hacke " + Abbau_Dauer(B_EISEN, G_STEINHACKE, H) + ", laub axt " + Abbau_Dauer(B_LAUB, G_STEINAXT, H)
Print "ertrag mit hacke: stein " + dg_name(Block_Ertrag(B_STEIN, G_STEINHACKE, H)) + ", kohle " + dg_name(Block_Ertrag(B_KOHLE, G_STEINHACKE, H)) + ", sandstein " + dg_name(Block_Ertrag(B_SANDSTEIN, G_STEINHACKE, H)) + ", eisen " + dg_name(Block_Ertrag(B_EISEN, G_STEINHACKE, H)) + ", stein kaputt " + dg_name(Block_Ertrag(B_STEIN, G_STEINHACKE, 0)) + ", stein axt " + dg_name(Block_Ertrag(B_STEIN, G_STEINAXT, H))
Print "stein mit hacke: " + Halten(8, B_STEIN, 7, 0.1, G_STEINHACKE, H)
Abbau_Loslassen()
Print "hinweis hand: " + Ertrag_Hinweis(B_STEIN, B_LUFT, 0)
Print "hinweis ast: " + Ertrag_Hinweis(B_KOHLE, G_AST, 0)
Print "hinweis kaputt: " + Ertrag_Hinweis(B_STEIN, G_STEINHACKE, 0)
Print "hinweis eisen: " + Ertrag_Hinweis(B_EISEN, G_STEINHACKE, H)
Print "hinweis erde: [" + Ertrag_Hinweis(B_ERDE, B_LUFT, 0) + "]"

; Verschleiss: ein Punkt je Aufruf, bei 0 kaputt, danach bleibt es so
Inventar_Leeren()
Inventar_Hinzu(G_STEINHACKE, 1, 2)
Inventar_Hinzu(G_AST, 5)
a1 = Werkzeug_Abnutzen(0) : h1 = inv_haltbar(0)
a2 = Werkzeug_Abnutzen(0) : h2 = inv_haltbar(0)
a3 = Werkzeug_Abnutzen(0) : h3 = inv_haltbar(0)
a4 = Werkzeug_Abnutzen(1)
Print "abnutzen: " + a1 + "/" + h1 + " " + a2 + "/" + h2 + " " + a3 + "/" + h3 + ", ast " + a4 + " " + inv_anzahl(1) + ", leer " + Werkzeug_Abnutzen(5)
Halten(3, B_STEIN, 10)
Print "anderer block faengt neu an: " + Halten(4, B_STEIN, 1)
Abbau_Loslassen()
Print "loslassen setzt zurueck, stufe " + Abbau_Stufe()
Print "grund  " + Halten(5, B_GRUND, 30)
Abbau_Loslassen()
Print "laub bricht: " + Halten(6, B_LAUB, 3)
Print "pause nach dem bruch: " + Halten(7, B_LAUB, 7)

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
