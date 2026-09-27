; 3D-19 - Animation: SetAnimKey, AddAnimSeq, Animate (Schleife, Pingpong,
; einmal, rueckwaerts, Uebergang), SetAnimTime, ExtractAnimSeq, AnimSeq,
; AnimLength, AnimTime, Animating, Hierarchie, CopyEntity, versteckt.
;
; Gemessen am Original (2026-09-26). Werte x1000 ueber Floor, damit weder die
; Rundung von Int (BUG-95) noch Str(float) (BUG-68) mitspielt.

Graphics3D 320,240,0,2
cam = CreateCamera()

Function F$(x#)
	Return Int(Floor(x * 1000.0 + 0.5))
End Function
Function Lage$(e)
	Return F(EntityX(e)) + "," + F(EntityY(e)) + "," + F(EntityZ(e)) + " r " + F(EntityPitch(e)) + "," + F(EntityYaw(e)) + "," + F(EntityRoll(e)) + " s " + Skala(e)
End Function
Function Skala$(e)
	TFormVector 1,0,0,e,0
	Return F(Sqr(TFormedX()*TFormedX() + TFormedY()*TFormedY() + TFormedZ()*TFormedZ()))
End Function
Function Stand$(e)
	Return "seq " + AnimSeq(e) + " len " + AnimLength(e) + " t " + F(AnimTime(e)) + " an " + Animating(e)
End Function
Function Z(s$)
	Print s
End Function

; --- 1) ohne Animation
w = CreateCube()
Z "1 ohne: " + Stand(w)

; --- 2) Schluessel setzen, Sequenz 0 anlegen
RotateEntity w,0,0,0 : PositionEntity w,0,0,0 : SetAnimKey w,0
RotateEntity w,0,90,0 : PositionEntity w,0,0,10 : SetAnimKey w,10
RotateEntity w,0,180,0 : PositionEntity w,10,0,10 : SetAnimKey w,20
s0 = AddAnimSeq(w,20)
Z "2 seq0=" + s0 + " " + Stand(w) + " | " + Lage(w)

; --- 3) zweite Sequenz nur mit Skalierung
ScaleEntity w,1,1,1 : SetAnimKey w,0,0,0,1
ScaleEntity w,3,1,1 : SetAnimKey w,4,0,0,1
s1 = AddAnimSeq(w,4)
Z "3 seq1=" + s1 + " " + Stand(w) + " | " + Lage(w)

; --- 4) Schleife
Animate w,1,3,0
Z "4 start " + Stand(w) + " | " + Lage(w)
For i = 1 To 8
	UpdateWorld
	Z "4." + i + " " + Stand(w) + " | " + Lage(w)
Next

; --- 5) Pingpong
Animate w,2,7,0
s$ = ""
For i = 1 To 8
	UpdateWorld
	s = s + " " + F(AnimTime(w))
Next
Z "5 pingpong" + s + " | " + Lage(w)

; --- 6) einmal
Animate w,3,6,0
s = ""
For i = 1 To 5
	UpdateWorld
	s = s + " " + F(AnimTime(w)) + "/" + Animating(w)
Next
Z "6 einmal" + s + " | " + Lage(w)

; --- 7) rueckwaerts, einmal
Animate w,3,-6,0
Z "7 start " + Stand(w)
s = ""
For i = 1 To 5
	UpdateWorld
	s = s + " " + F(AnimTime(w)) + "/" + Animating(w)
Next
Z "7 rueck" + s + " | " + Lage(w)

; --- 8) rueckwaerts, Schleife
Animate w,1,-7,0
s = ""
For i = 1 To 4
	UpdateWorld
	s = s + " " + F(AnimTime(w))
Next
Z "8 rueck schleife" + s

; --- 9) UpdateWorld mit Zeitschritt
Animate w,1,2,0
UpdateWorld 0.25
Z "9 halber schritt " + Stand(w)

; --- 10) SetAnimTime
SetAnimTime w,5
Z "10a " + Stand(w) + " | " + Lage(w)
SetAnimTime w,15
Z "10b " + Stand(w) + " | " + Lage(w)
SetAnimTime w,47
Z "10c " + Stand(w) + " | " + Lage(w)
SetAnimTime w,-3
Z "10d " + Stand(w) + " | " + Lage(w)
SetAnimTime w,2,1
Z "10e " + Stand(w) + " | " + Lage(w)
UpdateWorld
Z "10f nach update " + Stand(w)

; --- 11) Sequenz 1 skaliert nur
Animate w,1,1,1
UpdateWorld
Z "11 seq1 " + Stand(w) + " | " + Lage(w)

; --- 12) Uebergang
PositionEntity w,0,0,0 : RotateEntity w,0,0,0 : ScaleEntity w,1,1,1
SetAnimTime w,20,0
Z "12 ziel " + Lage(w)
PositionEntity w,0,0,0 : RotateEntity w,0,0,0 : ScaleEntity w,2,2,2
Animate w,3,0,0,4
Z "12 start " + Stand(w) + " | " + Lage(w)
For i = 1 To 5
	UpdateWorld
	Z "12." + i + " " + Stand(w) + " | " + Lage(w)
Next

; --- 13) Uebergang mit laufender Sequenz danach
PositionEntity w,0,0,0 : RotateEntity w,0,0,0
Animate w,1,2,0,2
s = ""
For i = 1 To 4
	UpdateWorld
	s = s + " [" + Stand(w) + " | " + Lage(w) + "]"
Next
Z "13" + s

; --- 14) anhalten, ungueltige Sequenz
Animate w,0
Z "14a " + Stand(w)
Animate w,1,1,7
Z "14b " + Stand(w)
Animate w,1,0,0
Z "14c " + Stand(w) + " | " + Lage(w)

; --- 15) ExtractAnimSeq
s2 = ExtractAnimSeq(w,5,15)
Z "15a seq2=" + s2 + " " + Stand(w)
SetAnimTime w,0,2
Z "15b " + Stand(w) + " | " + Lage(w)
SetAnimTime w,8,2
Z "15c " + Stand(w) + " | " + Lage(w)
s3 = ExtractAnimSeq(w,0,20,0)
SetAnimTime w,15,3
Z "15d seq3=" + s3 + " " + Stand(w) + " | " + Lage(w)

; --- 16) Hierarchie: Schluessel am Kind, Sequenz am Elternteil
p = CreatePivot()
k = CreateCube(p)
PositionEntity k,0,0,0 : SetAnimKey k,0
PositionEntity k,0,5,0 : SetAnimKey k,10
PositionEntity p,1,0,0 : SetAnimKey p,0
PositionEntity p,1,0,0 : SetAnimKey p,10
q0 = AddAnimSeq(p,10)
Z "16a seq=" + q0 + " eltern " + Stand(p) + " kind " + Stand(k)
Animate p,1,2
UpdateWorld : UpdateWorld
Z "16b kind " + F(EntityY(k)) + " welt " + F(EntityX(k,True)) + "," + F(EntityY(k,True))

; --- 17) CopyEntity
c = CopyEntity(p)
ck = GetChild(c,1)
Z "17a kopie " + Stand(c) + " kind " + F(EntityY(ck))
Animate c,1,1
UpdateWorld
Z "17b kopie " + Stand(c) + " kind " + F(EntityY(ck)) + " | vorlage " + Stand(p) + " kind " + F(EntityY(k))

; --- 18) versteckt: die Zeit steht
HideEntity p
UpdateWorld
Z "18a versteckt " + Stand(p)
ShowEntity p
UpdateWorld
Z "18b wieder " + Stand(p)

; --- 19) Kind versteckt, Elternteil animiert es weiter
HideEntity k
UpdateWorld
Z "19 kind versteckt " + Stand(p) + " kind " + F(EntityY(k))
ShowEntity k

; --- 20) AddAnimSeq an einer Entity, die schon einen Animator hat
PositionEntity k,0,0,0 : SetAnimKey k,0
PositionEntity k,0,-4,0 : SetAnimKey k,6
q1 = AddAnimSeq(p,6)
SetAnimTime p,3,q1
Z "20 seq=" + q1 + " " + Stand(p) + " kind " + F(EntityY(k)) + " eltern " + F(EntityX(p))

; --- 21) Animate ohne Pos-Schluessel laesst die Lage stehen
PositionEntity p,7,7,7
Animate p,1,1,q1
UpdateWorld
Z "21 eltern " + F(EntityX(p)) + "," + F(EntityY(p)) + " kind " + F(EntityY(k))

; --- 22) Sequenz ohne Schluessel, Laenge 0
e = CreatePivot()
n = AddAnimSeq(e,0)
Animate e,1,1
UpdateWorld
Z "22 leer seq=" + n + " " + Stand(e)

; --- 23) Zeiten zwischen zwei Schluesseln nicht ganzzahlig
g = CreatePivot()
PositionEntity g,0,0,0 : SetAnimKey g,0
PositionEntity g,10,0,0 : SetAnimKey g,3
AddAnimSeq g,3
SetAnimTime g,1.5
Z "23 " + F(EntityX(g))
SetAnimTime g,2.9
Z "23b " + F(EntityX(g))
