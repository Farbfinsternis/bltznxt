; Blockwelt - Rauschen
;
; Perlin-Rauschen ("improved noise", Ken Perlin 2002) in 2D und 3D, dazu
; fraktales Rauschen aus mehreren Oktaven. Die Permutationstabelle haengt an
; der Saat der Welt: dieselbe Saat ergibt dieselbe Welt.
;
; Bewusst ohne Ganzzahl-Hash mit grossen Faktoren - Blitz rechnet mit 32-Bit-
; Ganzzahlen, und ein Ueberlauf ist dort kein verlaesslicher Baustein.

Dim rs_p(511)

Function Rauschen_Saat(saat)
	SeedRnd saat
	For i = 0 To 255
		rs_p(i) = i
	Next
	For i = 255 To 1 Step -1
		j = Rand(0, i)
		t = rs_p(i) : rs_p(i) = rs_p(j) : rs_p(j) = t
	Next
	For i = 0 To 255
		rs_p(256 + i) = rs_p(i)
	Next
End Function

Function Rs_Grad#(h, x#, y#, z#)
	Local u#, v#
	h = h And 15
	If h < 8 Then u# = x Else u = y
	If h < 4
		v# = y
	ElseIf h = 12 Or h = 14
		v = x
	Else
		v = z
	EndIf
	If h And 1 Then u = -u
	If h And 2 Then v = -v
	Return u + v
End Function

; Werte etwa zwischen -1 und 1, meist zwischen -0.5 und 0.5.
Function Rauschen3#(x#, y#, z#)
	fx# = Floor(x) : fy# = Floor(y) : fz# = Floor(z)
	xi = Int(fx) And 255 : yi = Int(fy) And 255 : zi = Int(fz) And 255
	x = x - fx : y = y - fy : z = z - fz
	u# = x * x * x * (x * (x * 6 - 15) + 10)
	v# = y * y * y * (y * (y * 6 - 15) + 10)
	w# = z * z * z * (z * (z * 6 - 15) + 10)

	a = rs_p(xi) + yi : aa = rs_p(a) + zi : ab = rs_p(a + 1) + zi
	b = rs_p(xi + 1) + yi : ba = rs_p(b) + zi : bb = rs_p(b + 1) + zi

	g1# = Rs_Grad(rs_p(aa), x, y, z)
	g2# = Rs_Grad(rs_p(ba), x - 1, y, z)
	g3# = Rs_Grad(rs_p(ab), x, y - 1, z)
	g4# = Rs_Grad(rs_p(bb), x - 1, y - 1, z)
	g5# = Rs_Grad(rs_p(aa + 1), x, y, z - 1)
	g6# = Rs_Grad(rs_p(ba + 1), x - 1, y, z - 1)
	g7# = Rs_Grad(rs_p(ab + 1), x, y - 1, z - 1)
	g8# = Rs_Grad(rs_p(bb + 1), x - 1, y - 1, z - 1)

	l1# = g1 + u * (g2 - g1)
	l2# = g3 + u * (g4 - g3)
	l3# = g5 + u * (g6 - g5)
	l4# = g7 + u * (g8 - g7)
	m1# = l1 + v * (l2 - l1)
	m2# = l3 + v * (l4 - l3)
	Return m1 + w * (m2 - m1)
End Function

; 2D: ein Schnitt durch das 3D-Rauschen. Die Ebene liegt nicht auf einer
; ganzen Zahl, dort waere jeder Gitterpunkt null.
Function Rauschen2#(x#, z#, ebene#)
	Return Rauschen3(x, ebene + 0.371, z)
End Function

; Fraktales Rauschen: Oktaven mit doppelter Frequenz und halber Staerke,
; auf etwa -1 bis 1 gebracht.
Function Fbm2#(x#, z#, oktaven, ebene#)
	summe# = 0 : staerke# = 1 : gesamt# = 0
	For i = 1 To oktaven
		summe = summe + Rauschen2(x, z, ebene + i * 17) * staerke
		gesamt = gesamt + staerke
		x = x * 2 : z = z * 2 : staerke = staerke * 0.5
	Next
	Return summe / gesamt * 1.6
End Function
