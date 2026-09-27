; 3D-19 - LoadAnimMesh und LoadAnimSeq fuer .3ds: Objekte werden Entities,
; der Keyframer haengt sie um (Knotennummern, $$$DUMMY, Drehpunkt) und
; liefert die Schluessel; Drehschluessel sind relativ zum vorigen.
;
; Die Dateien erzeugt scripts/make_3ds_asset.py (test_anim*.3ds). Gemessen
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
	b$ = ""
	If CountSurfaces(e) > 0 Then b = " w " + F(MeshWidth(e)) + " h " + F(MeshHeight(e)) + " d " + F(MeshDepth(e))
	Z s + "[" + EntityName(e) + "] kinder " + CountChildren(e) + " fl " + CountSurfaces(e) + b + " | " + L(e) + " | " + Stand(e)
	For i = 1 To CountChildren(e)
		Baum(GetChild(e,i), tiefe + 1)
	Next
End Function

; --- 1) Aufbau nach dem Laden
m = LoadAnimMesh("tests/assets/test_anim.3ds")
Baum(m, 0)
arm = FindChild(m, "arm") : gelenk = FindChild(m, "gelenk") : hand = FindChild(m, "hand")

; --- 2) Sequenz 0 abtasten
For t = 0 To 5
	SetAnimTime m, t * 4
	Z "2." + t + " " + Stand(m) + " arm " + L(arm) + " hand " + L(hand) + " welt " + W(hand)
Next

; --- 3) zweite Sequenz aus Datei
n = LoadAnimSeq(m, "tests/assets/test_anim_seq.3ds")
SetAnimTime m, 4, n
Z "3 seq=" + n + " " + Stand(m) + " arm " + L(arm) + " hand " + L(hand)

; --- 4) laufen lassen
Animate m,2,3,0
For i = 1 To 4 : UpdateWorld : Next
Z "4 " + Stand(m) + " arm " + L(arm)

; --- 5) LoadMesh derselben Datei
e = LoadMesh("tests/assets/test_anim.3ds")
Z "5 kinder " + CountChildren(e) + " fl " + CountSurfaces(e) + " w " + F(MeshWidth(e)) + " h " + F(MeshHeight(e)) + " d " + F(MeshDepth(e)) + " " + Stand(e)
