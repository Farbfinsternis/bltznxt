; 3D-19 - LoadAnimMesh und LoadAnimSeq fuer .x: Frames werden Entities,
; AnimationSet wird Sequenz 0, LoadAnimSeq ordnet ueber Namen zu, eine
; Loadermatrix wirkt auf Frames und Schluessel.
;
; Die Dateien sind handgeschrieben (tests/assets/test_anim_arm*.x). Gemessen
; am Original (2026-09-26). Werte x1000 ueber Floor.

Graphics3D 320,240,0,2

Function F$(x#)
	Return Int(Floor(x * 1000.0 + 0.5))
End Function
Function W$(e)
	Return F(EntityX(e,True)) + "," + F(EntityY(e,True)) + "," + F(EntityZ(e,True))
End Function
Function L$(e)
	Return F(EntityX(e)) + "," + F(EntityY(e)) + "," + F(EntityZ(e)) + " r " + F(EntityPitch(e)) + "," + F(EntityYaw(e)) + "," + F(EntityRoll(e))
End Function
Function Stand$(e)
	Return "seq " + AnimSeq(e) + " len " + AnimLength(e) + " t " + F(AnimTime(e)) + " an " + Animating(e)
End Function
Function Z(s$)
	Print s
End Function
Function Baum(e, tiefe)
	s$ = ""
	For i = 1 To tiefe : s = s + " " : Next
	Z s + "[" + EntityName(e) + "] " + EntityClass(e) + " kinder " + CountChildren(e) + " fl " + CountSurfaces(e) + " | " + L(e) + " | " + Stand(e)
	For i = 1 To CountChildren(e)
		Baum(GetChild(e,i), tiefe + 1)
	Next
End Function

; --- 1) Aufbau nach dem Laden: Bild 0, angehalten
m = LoadAnimMesh("tests/assets/test_anim_arm.x")
Baum(m, 0)
arm = FindChild(m, "Arm") : hand = FindChild(m, "Hand") : finger = FindChild(m, "Finger")
Z "1 finger " + W(finger) + " hand breite " + F(MeshWidth(hand))

; --- 2) Sequenz 0 abtasten
For t = 0 To 4
	SetAnimTime m, t * 6
	Z "2." + t + " " + Stand(m) + " arm " + L(arm) + " finger " + W(finger)
Next

; --- 3) zweite Sequenz aus Datei
n = LoadAnimSeq(m, "tests/assets/test_anim_arm_seq.x")
Z "3 seq=" + n + " kinder " + CountChildren(m)
SetAnimTime m, 4, 1
Z "3b " + Stand(m) + " arm " + L(arm) + " hand " + L(hand) + " finger " + W(finger)
Animate m,1,1,1
For i = 1 To 3 : UpdateWorld 2 : Next
Z "3c " + Stand(m) + " hand " + L(hand)

; --- 4) LoadAnimSeq auf einer Entity ohne Animator, fehlende Datei
c = CreateCube()
Z "4 " + LoadAnimSeq(c, "tests/assets/test_anim_arm_seq.x") + " " + LoadAnimSeq(m, "tests/assets/gibtsnicht.x") + " " + Stand(m)

; --- 5) LoadMesh schmilzt alles ein, ohne Animator
e = LoadMesh("tests/assets/test_anim_arm.x")
Z "5 kinder " + CountChildren(e) + " fl " + CountSurfaces(e) + " breite " + F(MeshWidth(e)) + " " + Stand(e)

; --- 6) Loadermatrix: y und z getauscht
LoaderMatrix "x",1,0,0, 0,0,1, 0,1,0
g = LoadAnimMesh("tests/assets/test_anim_arm.x")
ga = FindChild(g, "Arm") : gh = FindChild(g, "Hand") : gf = FindChild(g, "Finger")
Z "6 hand " + L(gh) + " finger " + W(gf)
SetAnimTime g, 5
Z "6b arm " + L(ga) + " finger " + W(gf)
SetAnimTime g, 15
Z "6c arm " + L(ga) + " finger " + W(gf)
LoaderMatrix "x",1,0,0, 0,1,0, 0,0,1

; --- 7) ExtractAnimSeq auf geladenen Schluesseln
x = ExtractAnimSeq(m, 10, 20)
SetAnimTime m, 5, x
Z "7 seq=" + x + " " + Stand(m) + " arm " + L(arm)
