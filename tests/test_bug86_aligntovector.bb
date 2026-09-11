; BUG-86 - AlignToVector dreht die vorhandene Lage, es baut sie nicht neu.
;
; Referenz (`bbruntime/bbblitz3d.cpp:1855`): das Original nimmt die
; **vorhandene** Weltrotation, sucht deren Achse (i, j oder k) und dreht sie
; auf dem kuerzesten Weg auf das Ziel:
;
;   dp = ax . tv          wie weit ist es noch
;   cp = ax x tv          worum gedreht wird
;   neue Rotation = Quat(Winkel um cp) * alte
;
; Dass die vorhandene Lage der Ausgangspunkt ist, ist der ganze Punkt: die
; beiden uebrigen Freiheitsgrade bleiben, wie sie waren. Unsere frueherere
; Fassung setzte Eulerwinkel aus zwei atan2 je Achse neu zusammen und warf
; die Ausgangslage weg - sie traf 2 von 15 gemessenen Winkeltripeln.
;
; **Geprueft wird an den Achsen, nicht an den Eulerwinkeln.** Im Kippfall
; (Nick +-90) sind Eulerwinkel mehrdeutig: das Original meldet dort
; (90,180,-180), wir (90,0,0) - dieselbe Orientierung, zwei Namen. Wohin die
; Achsen zeigen, ist dagegen eindeutig, und dafuer gibt es seit heute
; TFormVector. Gemessen: 21 von 21 Achsen stimmen zwischen beiden Systemen
; bis auf ein Tausendstel.
;
; Die Zusicherungen unten sind bewusst **Invarianten** statt abgeschriebener
; Zahlen - sie gelten in beiden Systemen aus sich heraus.

Graphics3D 640, 480, 16, 2
SetBuffer BackBuffer()

; Liefert das Skalarprodukt der Weltachse `welche` von `e` mit (x,y,z).
Function AchsePunkt#(e, welche, x#, y#, z#)
	If welche = 1 Then TFormVector 1,0,0,e,0
	If welche = 2 Then TFormVector 0,1,0,e,0
	If welche = 3 Then TFormVector 0,0,1,e,0
	Return TFormedX#()*x + TFormedY#()*y + TFormedZ#()*z
End Function

; --- 1) Mit rate=1 zeigt die gewaehlte Achse genau in die Zielrichtung ---
;
; Fuer alle drei Achsen, und aus einer Ausgangslage, die nicht die Identitaet
; ist - sonst pruefte der Test die halbe Aussage.
For achse = 1 To 3
	e = CreatePivot()
	RotateEntity e, 10, 20, 30
	AlignToVector e, 0.6, 0, 0.8, achse
	UpdateWorld
	If AchsePunkt(e, achse, 0.6, 0, 0.8) > 0.9999 Then Print "achse " + achse + " zeigt hin" Else Print "FEHLER achse " + achse + " zeigt hin"
	FreeEntity e
Next

; Auch ein nicht normierter Zielvektor wird angenommen - das Original teilt
; selbst durch die Laenge.
e = CreatePivot()
AlignToVector e, 0, 7, 0, 2
UpdateWorld
If AchsePunkt(e, 2, 0, 1, 0) > 0.9999 Then Print "nicht normiert" Else Print "FEHLER nicht normiert"
FreeEntity e

; --- 2) Die Lage bleibt orthonormal ---
;
; Wer Eulerwinkel neu zusammensetzt, kann das verlieren; wer dreht, nicht.
e = CreatePivot()
RotateEntity e, 33, -47, 71
AlignToVector e, 1, 1, 1, 3
UpdateWorld
TFormVector 1,0,0,e,0
ix# = TFormedX#() : iy# = TFormedY#() : iz# = TFormedZ#()
TFormVector 0,1,0,e,0
jx# = TFormedX#() : jy# = TFormedY#() : jz# = TFormedZ#()
If Abs(ix*jx + iy*jy + iz*jz) < 0.001 Then Print "achsen senkrecht" Else Print "FEHLER achsen senkrecht"
If Abs(Sqr(ix*ix + iy*iy + iz*iz) - 1) < 0.001 Then Print "achse hat laenge eins" Else Print "FEHLER achse hat laenge eins"
FreeEntity e

; --- 3) Es wird auf dem KUERZESTEN Weg gedreht ---
;
; Der Kern des Befehls: die beiden anderen Freiheitsgrade bleiben stehen.
; Zeigt die Achse schon dorthin, passiert gar nichts - auch nicht an den
; uebrigen Achsen. Das Original steigt dafuer mit `dp >= 1-EPSILON` aus.
e = CreatePivot()
RotateEntity e, 0, 40, 0
UpdateWorld
vorher# = AchsePunkt(e, 1, 1, 0, 0)      ; wo die x-Achse gerade steht
TFormVector 0,0,1,e,0
kx# = TFormedX#() : ky# = TFormedY#() : kz# = TFormedZ#()
AlignToVector e, kx, ky, kz, 3           ; z-Achse auf sich selbst
UpdateWorld
If Abs(AchsePunkt(e, 1, 1, 0, 0) - vorher) < 0.001 Then Print "schon ausgerichtet aendert nichts" Else Print "FEHLER schon ausgerichtet aendert nichts"
FreeEntity e

; --- 4) rate teilt den Winkel auf ---
;
; Das Original dreht um acos(dp) * rate. Bei rate 0.5 bleibt also genau der
; halbe Winkel stehen: aus 90 Grad werden 45.
e = CreatePivot()
AlignToVector e, 1, 0, 0, 3, 0.5
UpdateWorld
; die z-Achse stand auf (0,0,1), das Ziel ist (1,0,0) - 90 Grad.
; Nach der halben Drehung steht sie auf 45 Grad zu beiden.
If Abs(AchsePunkt(e, 3, 1, 0, 0) - 0.7071) < 0.002 Then Print "halbe drehung" Else Print "FEHLER halbe drehung"
FreeEntity e

; Zweimal die halbe Drehung kommt dem Ziel naeher, erreicht es aber nicht -
; so wird der Befehl in einer Schleife benutzt.
e = CreatePivot()
AlignToVector e, 1, 0, 0, 3, 0.5
UpdateWorld
eins# = AchsePunkt(e, 3, 1, 0, 0)
AlignToVector e, 1, 0, 0, 3, 0.5
UpdateWorld
zwei# = AchsePunkt(e, 3, 1, 0, 0)
If zwei > eins And zwei < 0.9999 Then Print "zweimal halb" Else Print "FEHLER zweimal halb"
FreeEntity e

; rate 0 aendert nichts.
e = CreatePivot()
RotateEntity e, 0, 40, 0
UpdateWorld
vorher2# = AchsePunkt(e, 3, 1, 0, 0)
AlignToVector e, 1, 0, 0, 3, 0
UpdateWorld
If Abs(AchsePunkt(e, 3, 1, 0, 0) - vorher2) < 0.001 Then Print "rate null" Else Print "FEHLER rate null"
FreeEntity e

; --- 5) Der Fall genau entgegengesetzt ---
;
; Hier gibt das Kreuzprodukt keine Drehachse her. Das Original nimmt dann die
; naechste Achse der Entity selbst und dreht um eine halbe Umdrehung.
e = CreatePivot()
AlignToVector e, 0, 0, -1, 3
UpdateWorld
If AchsePunkt(e, 3, 0, 0, -1) > 0.9999 Then Print "gegenrichtung" Else Print "FEHLER gegenrichtung"
FreeEntity e

; --- 6) Die vollstaendige Lage gegen gemessene Werte ---
;
; Die Zusicherungen oben sind Invarianten - sie pruefen die Semantik, aber
; nicht, ob die **beiden anderen** Achsen dort landen, wo das Original sie
; hinlegt. Genau daran scheiterte die alte Fassung: sie warf die Ausgangslage
; weg, und ein Invariantentest allein hat das nur zum Teil bemerkt (die
; Gegenprobe mit dem alten Stand fiel bei 3 von 12 Zeilen durch, die
; Winkelmessung dagegen bei 13 von 15).
;
; Darum hier alle drei Weltachsen gegen am Original gemessene Tausendstel,
; jeweils aus der Ausgangslage (10,20,30) heraus.
Function AchseStimmt(e, welche, sx#, sy#, sz#)
	If welche = 1 Then TFormVector 1,0,0,e,0
	If welche = 2 Then TFormVector 0,1,0,e,0
	If welche = 3 Then TFormVector 0,0,1,e,0
	If Abs(TFormedX#() - sx) > 0.002 Then Return 0
	If Abs(TFormedY#() - sy) > 0.002 Then Return 0
	If Abs(TFormedZ#() - sz) > 0.002 Then Return 0
	Return 1
End Function

e = CreatePivot()
RotateEntity e, 10, 20, 30
AlignToVector e, 1, 0, 0, 1
UpdateWorld
If AchseStimmt(e,1, 1,0,0) And AchseStimmt(e,2, 0,0.997,0.081) And AchseStimmt(e,3, 0,-0.081,0.997) Then Print "volle lage achse 1" Else Print "FEHLER volle lage achse 1"
FreeEntity e

e = CreatePivot()
RotateEntity e, 10, 20, 30
AlignToVector e, 0, 0, 1, 2
UpdateWorld
If AchseStimmt(e,1, 0.987,0.160,0) And AchseStimmt(e,2, 0,0,1) And AchseStimmt(e,3, 0.160,-0.987,0) Then Print "volle lage achse 2" Else Print "FEHLER volle lage achse 2"
FreeEntity e

e = CreatePivot()
RotateEntity e, 10, 20, 30
AlignToVector e, 0, 1, 0, 3
UpdateWorld
If AchseStimmt(e,1, 0.985,0,-0.174) And AchseStimmt(e,2, -0.174,0,-0.985) And AchseStimmt(e,3, 0,1,0) Then Print "volle lage achse 3" Else Print "FEHLER volle lage achse 3"
FreeEntity e

; Eine schraege Richtung, bei der keine Komponente wegfaellt.
e = CreatePivot()
RotateEntity e, 10, 20, 30
AlignToVector e, -1, -1, -1, 3
UpdateWorld
If AchseStimmt(e,1, -0.364,-0.451,0.815) And AchseStimmt(e,2, -0.731,0.681,0.050) And AchseStimmt(e,3, -0.577,-0.577,-0.577) Then Print "volle lage schraeg" Else Print "FEHLER volle lage schraeg"
FreeEntity e

; --- 7) Ein Nullvektor tut nichts ---
e = CreatePivot()
RotateEntity e, 0, 40, 0
UpdateWorld
vorher3# = AchsePunkt(e, 3, 1, 0, 0)
AlignToVector e, 0, 0, 0, 3
UpdateWorld
If Abs(AchsePunkt(e, 3, 1, 0, 0) - vorher3) < 0.001 Then Print "nullvektor" Else Print "FEHLER nullvektor"
FreeEntity e

Print "fertig"
