; Friendly Fire - die Spielerfiguren (Mehrspieler, Schritt 2).
;
; Eine Figur ist ein Roboter aus Teilen an festen Gelenken (daten/figur.glb,
; werkzeug/figur.py). Die Datei enthaelt keine Animation: dieses Modul dreht
; die Teile selbst - Laufen aus der Geschwindigkeit, Zielen aus dem
; Blickwinkel, Rueckstoss beim Schuss, Umfallen beim Tod. Eine andere Figur
; (frei aus einem Paket, von Hand) laeuft im Spiel, wenn ihre Teile wieder so
; heissen: bein_l, bein_r, torso, kopf, arm_l, arm_r, waffe, name.
;
; Jede Figur bekommt eine Farbe (EntityColor auf die weissen Teile; Stiefel,
; Visier und Handschuhe bleiben dunkel), ein Waffenmodell vor der Brust und
; ein Namensschild ueber dem Kopf. Alle Teile sind fuer LinePick da
; (EntityPickMode 2) - Figur_Von liefert zu einem getroffenen Teil die Figur,
; Figur_Kopf sagt, ob es der Kopf war.
;
;   Figuren_Laden ordner$                 ; figur.glb und die Waffen mg, rl, rail
;   f.Figur = Figur_Neu(r, g, b, name$)
;   Figur_Setzen f, x, y, z, gier, nick   ; Fuesse, Blick; nick + = nach unten
;   Figur_Takt f, tempo, boden            ; je Takt (1/60 s), danach
;   Figur_Waffe f, nr                     ; 1 MG, 2 Raketenwerfer, 3 Railgun, 0 keine
;   Figur_Feuern f                        ; Rueckstoss
;   Figur_Sterben f / Figur_Beleben f
;   Figur_Weg f

Const FG_PHASE# = 120.0         ; Grad der Schrittphase je Meter: ein Doppelschritt 3 m
Const FG_LAUF# = 10.3           ; ab dieser Geschwindigkeit volle Schrittweite
Const FG_SCHWUNG# = 50.0        ; Grad, mit denen die Beine schwingen
Const FG_STERBEN# = 0.6         ; Sekunden bis die Figur liegt
Const FG_RUECK# = 0.15          ; Sekunden Rueckstoss
Const FG_WAFFE# = 1.7          ; Waffenmodelle sind fuer die Hand der Ich-Sicht gebaut

Global fg_vorlage, fg_schrift
Dim fg_waffe_vorlage(3)

Type Figur
	Field ent                                   ; Wurzel, Fuesse bei y = 0
	Field torso, kopf, arm_l, arm_r, bein_l, bein_r
	Field anker_waffe, anker_name
	Field w1, w2, w3                            ; Waffenmodelle (nur eines zu sehen)
	Field waffe
	Field schild, tafel                         ; Sprite und Textur des Namens
	Field phase#                                ; Schrittphase in Grad
	Field x#, y#, z#                            ; Fuesse
	Field gier#, nick#                          ; Blickrichtung; nick + = nach unten, Grad
	Field lebt
	Field tod#                                  ; 0 steht ... 1 liegt
	Field rueck#                                ; > 0: Rueckstoss, so viele Sekunden noch
	Field r, g, b
End Type

Function Figuren_Laden(ordner$)
	fg_vorlage = LoadAnimMesh(ordner + "/figur.glb")
	If fg_vorlage = 0 Then RuntimeError "Figur nicht lesbar: " + ordner + "/figur.glb"
	HideEntity fg_vorlage
	fg_schrift = LoadFont("Arial", 34, True)
	Restore Figuren_Waffen
	For i = 1 To 3
		Read datei$
		fg_waffe_vorlage(i) = LoadMesh(ordner + "/" + datei)
		If fg_waffe_vorlage(i) = 0 Then RuntimeError "Waffe nicht lesbar: " + ordner + "/" + datei
		HideEntity fg_waffe_vorlage(i)
	Next
End Function

.Figuren_Waffen
Data "mg.glb", "rl.glb", "rail.glb"

Function Figur_Neu.Figur(r, g, b, name$)
	f.Figur = New Figur
	f\ent = CopyEntity(fg_vorlage)
	ShowEntity f\ent
	f\torso = FindChild(f\ent, "torso") : f\kopf = FindChild(f\ent, "kopf")
	f\arm_l = FindChild(f\ent, "arm_l") : f\arm_r = FindChild(f\ent, "arm_r")
	f\bein_l = FindChild(f\ent, "bein_l") : f\bein_r = FindChild(f\ent, "bein_r")
	f\anker_waffe = FindChild(f\ent, "waffe") : f\anker_name = FindChild(f\ent, "name")
	If f\torso = 0 Or f\kopf = 0 Or f\arm_l = 0 Or f\arm_r = 0 Or f\bein_l = 0 Or f\bein_r = 0 Or f\anker_waffe = 0 Or f\anker_name = 0
		RuntimeError "Figur ohne alle Teile (bein_l, bein_r, torso, kopf, arm_l, arm_r, waffe, name)"
	EndIf
	f\r = r : f\g = g : f\b = b
	EntityColor f\torso, r, g, b : EntityColor f\kopf, r, g, b
	EntityColor f\arm_l, r, g, b : EntityColor f\arm_r, r, g, b
	EntityColor f\bein_l, r, g, b : EntityColor f\bein_r, r, g, b
	Figur_Pickbar(f\ent)
	f\w1 = Figur_Waffenmodell(f, 1) : f\w2 = Figur_Waffenmodell(f, 2) : f\w3 = Figur_Waffenmodell(f, 3)
	f\waffe = 0
	f\lebt = True
	Figur_Name(f, name)
	Figur_Pose(f, 0, True)
	Return f
End Function

; Alles unter e pickbar machen, auf die Dreiecke genau
Function Figur_Pickbar(e)
	EntityPickMode e, 2
	For i = 1 To CountChildren(e)
		Figur_Pickbar(GetChild(e, i))
	Next
End Function

Function Figur_Waffenmodell(f.Figur, nr)
	w = CopyEntity(fg_waffe_vorlage(nr), f\anker_waffe)
	ScaleEntity w, FG_WAFFE, FG_WAFFE, FG_WAFFE
	PositionEntity w, 0, 0, 0
	HideEntity w
	Return w
End Function

; Namensschild: ein Sprite ueber dem Kopf mit dem Namen als Textur (weisse
; Schrift auf Schwarz, Schwarz ist durchsichtig)
Function Figur_Name(f.Figur, name$)
	If f\schild Then FreeEntity f\schild : FreeTexture f\tafel
	f\tafel = CreateTexture(256, 64, 1 + 4)
	SetBuffer TextureBuffer(f\tafel)
	Color 0, 0, 0 : Rect 0, 0, 256, 64
	If fg_schrift Then SetFont fg_schrift
	Color 50, 50, 50 : Text 130, 34, name, True, True          ; Schatten, kein reines Schwarz
	Color 255, 255, 255 : Text 128, 32, name, True, True
	SetBuffer BackBuffer()
	f\schild = CreateSprite(f\anker_name)
	EntityTexture f\schild, f\tafel
	EntityFX f\schild, 1 + 8
	ScaleSprite f\schild, 0.9, 0.225
End Function

Function Figur_Waffe(f.Figur, nr)
	HideEntity f\w1 : HideEntity f\w2 : HideEntity f\w3
	f\waffe = nr
	Select nr
	Case 1 : ShowEntity f\w1
	Case 2 : ShowEntity f\w2
	Case 3 : ShowEntity f\w3
	End Select
End Function

Function Figur_Feuern(f.Figur)
	f\rueck = FG_RUECK
End Function

Function Figur_Sterben(f.Figur)
	f\lebt = False
End Function

Function Figur_Beleben(f.Figur)
	f\lebt = True : f\tod = 0 : f\rueck = 0
	Figur_Pose(f, 0, True)
End Function

; Fuesse bei (x,y,z), Blickrichtung gier, Blick nach oben/unten nick
Function Figur_Setzen(f.Figur, x#, y#, z#, gier#, nick#)
	f\x = x : f\y = y : f\z = z
	f\gier = gier : f\nick = nick
	PositionEntity f\ent, x, y, z
End Function

; Ein Takt: Schrittphase, Tod, Rueckstoss, dann die Pose
Function Figur_Takt(f.Figur, tempo#, boden)
	If f\lebt
		If boden Then f\phase = f\phase + tempo * TAKT * FG_PHASE
		If f\tod > 0 Then f\tod = f\tod - TAKT / FG_STERBEN : If f\tod < 0 Then f\tod = 0
	Else
		f\tod = f\tod + TAKT / FG_STERBEN
		If f\tod > 1 Then f\tod = 1
	EndIf
	If f\rueck > 0 Then f\rueck = f\rueck - TAKT : If f\rueck < 0 Then f\rueck = 0
	Figur_Pose(f, tempo, boden)
End Function

Function Figur_Pose(f.Figur, tempo#, boden)
	st# = tempo / FG_LAUF
	If st > 1 Then st = 1
	If Not f\lebt Then st = 0
	s# = Sin(f\phase) * FG_SCHWUNG * st
	If Not boden Then s = 0

	; Beine: Schritt; in der Luft leicht gespreizt
	If boden
		RotateEntity f\bein_l, s, 0, 0 : RotateEntity f\bein_r, -s, 0, 0
	Else
		RotateEntity f\bein_l, -18, 0, 0 : RotateEntity f\bein_r, 12, 0, 0
	EndIf

	; Rumpf und Kopf nehmen den Blick an, der Rumpf lehnt sich in den Lauf
	n# = f\nick
	If n > 60 Then n = 60
	If n < -60 Then n = -60
	PositionEntity f\torso, 0, 0.85 + 0.035 * Abs(Cos(f\phase)) * st, 0
	RotateEntity f\torso, n * 0.5 + 7 * st, 0, 0
	RotateEntity f\kopf, n * 0.5 - 7 * st, 0, 0

	; Arme: mit Waffe nach vorn auf die Blickrichtung, ohne gegenlaeufig zu den Beinen
	rueck# = 0
	If f\rueck > 0 Then rueck = 10 * f\rueck / FG_RUECK
	If f\waffe > 0 And f\lebt
		RotateEntity f\arm_r, -90 + n * 0.5 - 7 * st - rueck, -8, 0
		RotateEntity f\arm_l, -80 + n * 0.5 - 7 * st - rueck, 24, 0
		PositionEntity f\anker_waffe, 0.16, 0.36, 0.22 - 0.05 * rueck / 10
		RotateEntity f\anker_waffe, n * 0.5 - rueck * 0.6, 0, 0
	Else
		RotateEntity f\arm_l, -s * 0.8 - 7 * st, 0, 0 : RotateEntity f\arm_r, s * 0.8 - 7 * st, 0, 0
		If f\waffe > 0
			PositionEntity f\anker_waffe, 0.16, 0.36, 0.22
			RotateEntity f\anker_waffe, 0, 0, 0
		EndIf
	EndIf

	; Tod: auf den Ruecken fallen; die Wurzel dreht sich um die Fuesse
	If f\tod > 0
		t# = f\tod
		t = t * t * (3 - 2 * t)             ; weich
		RotateEntity f\ent, -90 * t, f\gier, 0
		PositionEntity f\ent, f\x, f\y + 0.13 * t, f\z      ; der Rumpf ist 0,25 m dick: nicht in den Boden
	Else
		RotateEntity f\ent, 0, f\gier, 0
		PositionEntity f\ent, f\x, f\y, f\z
	EndIf
End Function

; Figur, zu der ein getroffenes Teil gehoert (oder Null)
Function Figur_Von.Figur(e)
	While e <> 0
		For f.Figur = Each Figur
			If f\ent = e Then Return f
		Next
		e = GetParent(e)
	Wend
	Return Null
End Function

Function Figur_Kopf(f.Figur, e)
	While e <> 0
		If e = f\kopf Then Return True
		If e = f\ent Then Return False
		e = GetParent(e)
	Wend
	Return False
End Function

Function Figur_Weg(f.Figur)
	If f\schild Then FreeEntity f\schild : FreeTexture f\tafel
	FreeEntity f\ent
	Delete f
End Function
