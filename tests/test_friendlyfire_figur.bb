; Friendly Fire Mehrspieler Schritt 2 - die Spielerfigur.
;
; Laedt daten/figur.glb (samples/friendlyfire/werkzeug/figur.py) und die
; Waffenmodelle, stellt Figuren auf und prueft Teile, Pose beim Stehen, Laufen,
; Zielen, Schiessen und Sterben sowie die Treffer per LinePick. 60 Takte je
; Sekunde; Winkel in ganzen Grad, Laengen in Zentimetern.

Include "../samples/friendlyfire/karte.bb"
Include "../samples/friendlyfire/spieler.bb"
Include "../samples/friendlyfire/figuren.bb"

Graphics3D 320,240,0,2
SetBuffer BackBuffer()

Function C$(x#)
	Return Int(Floor(x * 100.0 + 0.5))
End Function

Function Takte(f.Figur, n, tempo#, boden)
	For i = 1 To n
		Figur_Takt(f, tempo, boden)
		UpdateWorld
	Next
End Function

Figuren_Laden("samples/friendlyfire/daten")
cam = CreateCamera()

a.Figur = Figur_Neu(255, 80, 80, "Anna")
b.Figur = Figur_Neu(80, 160, 255, "Ben")
n = 0
For f.Figur = Each Figur
	n = n + 1
Next
Print "laden:     figuren " + n + " waffe " + a\waffe + " lebt " + a\lebt + " tod " + C(a\tod)

; 1) Stehen: alles in Ruhe, die Beine gerade, der Rumpf bei 0,85 m
Figur_Setzen(a, 0, 0, 5, 0, 0)
Takte(a, 1, 0, True)
Print "1 stehen:  bein " + Int(EntityPitch(a\bein_l)) + "/" + Int(EntityPitch(a\bein_r)) + " arm " + Int(EntityPitch(a\arm_l)) + "/" + Int(EntityPitch(a\arm_r)) + " torso y " + C(EntityY(a\torso)) + " phase " + Int(a\phase)

; 2) Laufen: 30 Takte mit voller Geschwindigkeit sind 5,15 m = 618 Grad Schrittphase;
;    die Beine schwingen gegenlaeufig um 50 * sin(618) = -49 Grad, die Arme mit
Takte(a, 30, FG_LAUF, True)
Print "2 laufen:  phase " + Int(a\phase) + " bein " + Int(EntityPitch(a\bein_l)) + "/" + Int(EntityPitch(a\bein_r)) + " arm " + Int(EntityPitch(a\arm_l)) + "/" + Int(EntityPitch(a\arm_r)) + " torso " + Int(EntityPitch(a\torso))

; 3) Sprung: in der Luft bleibt die Phase stehen, die Beine sind gespreizt
ph = Int(a\phase)
Takte(a, 10, FG_LAUF, False)
Print "3 luft:    phase gleich " + (ph = Int(a\phase)) + " bein " + Int(EntityPitch(a\bein_l)) + "/" + Int(EntityPitch(a\bein_r))

; 4) Zielen mit dem Raketenwerfer: beide Arme nach vorn, der Blick nach unten nimmt sie mit
Takte(a, 1, 0, True)
Figur_Waffe(a, 2)
Figur_Setzen(a, 0, 0, 5, 0, 0)
Takte(a, 1, 0, True)
Print "4 zielen:  arm " + Int(EntityPitch(a\arm_l)) + "/" + Int(EntityPitch(a\arm_r)) + " waffe " + a\waffe
Figur_Setzen(a, 0, 0, 5, 0, 40)
Takte(a, 1, 0, True)
Print "4 nach unten: arm " + Int(EntityPitch(a\arm_l)) + "/" + Int(EntityPitch(a\arm_r)) + " torso " + Int(EntityPitch(a\torso)) + " kopf " + Int(EntityPitch(a\kopf)) + " anker " + Int(EntityPitch(a\anker_waffe))
Figur_Setzen(a, 0, 0, 5, 0, 0)

; 5) Rueckstoss: 9 Takte = 0,15 s, dann wieder ruhig
Figur_Feuern(a)
Takte(a, 1, 0, True)
r1 = Int(EntityPitch(a\arm_r))
Takte(a, 9, 0, True)
Print "5 rueck:   arm " + r1 + " -> " + Int(EntityPitch(a\arm_r)) + " rueck " + C(a\rueck)

; 6) Treffer mit LinePick: Rumpf, Kopf, daneben
Figur_Setzen(a, 0, 0, 5, 0, 0)
Takte(a, 1, 0, True)
Figur_Setzen(b, 4, 0, 5, 0, 0)
Takte(b, 1, 0, True)
e = LinePick(0, 1.2, 0, 0, 0, 10)
tf.Figur = Figur_Von(e)
Print "6 rumpf:   getroffen " + (e <> 0) + " figur a " + (tf = a) + " kopf " + Figur_Kopf(a, e) + " abstand " + C(PickedZ() - 0)
e = LinePick(0, 1.7, 0, 0, 0, 10)
tf = Figur_Von(e)
Print "6 kopf:    getroffen " + (e <> 0) + " figur a " + (tf = a) + " kopf " + Figur_Kopf(a, e)
e = LinePick(4, 1.2, 0, 0, 0, 10)
tf = Figur_Von(e)
Print "6 figur b: figur b " + (tf = b) + " nicht a " + (tf <> a)
e = LinePick(2, 1.2, 0, 0, 0, 10)
Print "6 daneben: getroffen " + (e <> 0)

; 7) Sterben: in 0,6 s = 36 Takten auf den Ruecken, der Kopf liegt dann hinter den Fuessen
Figur_Sterben(a)
Takte(a, 18, 0, True)
Print "7 sterben: halb tod " + C(a\tod) + " pitch " + Int(EntityPitch(a\ent))
Takte(a, 18, 0, True)
Print "7 tot:     tod " + C(a\tod) + " pitch " + Int(EntityPitch(a\ent)) + " kopf y " + C(EntityY(a\kopf, True)) + " z " + C(EntityZ(a\kopf, True) - 5) + " lebt " + a\lebt
Figur_Beleben(a)
Print "7 belebt:  tod " + C(a\tod) + " pitch " + Int(EntityPitch(a\ent)) + " lebt " + a\lebt

; 8) Blickrichtung: Gier 90 schaut nach -x (Blitz dreht positiv nach links)
Figur_Setzen(a, 0, 0, 5, 90, 0)
Takte(a, 1, 0, True)
TFormVector 0, 0, 1, a\ent, 0
Print "8 gier:    vorn " + C(TFormedX()) + "," + C(TFormedY()) + "," + C(TFormedZ())

Figur_Weg(a)
n = 0
For f.Figur = Each Figur
	n = n + 1
Next
Print "weg:       figuren " + n
End
