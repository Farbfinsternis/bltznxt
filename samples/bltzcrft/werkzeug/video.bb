; BLTZCRFT - Aufnahme fuer das Werbevideo: ein Kameraflug ueber die Welt,
; Bild fuer Bild mit festem Takt (30 Bilder je Sekunde), gespeichert als BMP.
; Vor jedem Bild wird alles nachgeladen, was in Sichtweite fehlt - so ruckelt
; die Aufnahme nicht, egal wie lange ein Bild dauert.
;
;   video.exe [zielordner] [bilder]
;   ohne Angaben: ../../../build/video, 900 Bilder (30 s)
;
; Die Route (Saat 2026) folgt einer Catmull-Rom-Kurve durch Wegpunkte:
; ueber dem Ozean im Norden, ueber Bucht und Strand aufs Land, an Tuempeln
; vorbei und an den Schneebergen entlang. Die Hoehe folgt dem Gelaende
; geglaettet, der Blick geht leicht nach unten voraus.
;
; Das Video setzt werkzeug/video_schnitt.py in Blender zusammen (Logo ein-
; und ausblenden, MP4).

Const SICHT = 12
Include "../rauschen.bb"
Include "../bloecke.bb"
Include "../welt.bb"
Include "../netz.bb"
Include "../spieler.bb"
Include "../laden.bb"

Const BREITE = 1920
Const HOEHE_BILD = 1080
Const BPS = 30
Const UEBER# = 16.0          ; Flughoehe ueber dem Gelaende
Const VORAUS# = 45.0         ; so weit voraus geht der Blick

Graphics3D BREITE, HOEHE_BILD, 0, 2
SetBuffer BackBuffer()

ziel$ = "../../../build/video"
bilder = 30 * BPS
a$ = Trim(CommandLine())
If a <> ""
	p = Instr(a, " ")
	If p
		ziel = Left(a, p - 1) : bilder = Int(Mid(a, p + 1))
	Else
		ziel = a
	EndIf
EndIf

Bloecke_Laden()
Netz_Laden()
Welt_Saat(2026)

; ---- Route ---------------------------------------------------------------

Dim wp_x#(15), wp_z#(15)
Restore route_daten
n_wp = 0
Repeat
	Read x#, z#
	If x = 9999 Then Exit
	wp_x(n_wp) = x : wp_z(n_wp) = z : n_wp = n_wp + 1
Forever

; Dicht abgetastet, mit aufsummierter Laenge - so laesst sich die Kurve mit
; gleichmaessigem Tempo abfliegen.
Const PROBEN = 4000
Dim rp_x#(PROBEN), rp_z#(PROBEN), rp_s#(PROBEN)
For i = 0 To PROBEN
	t# = Float(i) / PROBEN * (n_wp - 1)
	k = Int(Floor(t)) : If k > n_wp - 2 Then k = n_wp - 2
	u# = t - k
	k0 = k - 1 : If k0 < 0 Then k0 = 0
	k3 = k + 2 : If k3 > n_wp - 1 Then k3 = n_wp - 1
	rp_x(i) = Catmull(wp_x(k0), wp_x(k), wp_x(k + 1), wp_x(k3), u)
	rp_z(i) = Catmull(wp_z(k0), wp_z(k), wp_z(k + 1), wp_z(k3), u)
	If i > 0
		dx# = rp_x(i) - rp_x(i - 1) : dz# = rp_z(i) - rp_z(i - 1)
		rp_s(i) = rp_s(i - 1) + Sqr(dx * dx + dz * dz)
	EndIf
Next
laenge# = rp_s(PROBEN)
Print "Route " + Int(laenge) + " m, " + bilder + " Bilder, " + Int(laenge / (bilder / Float(BPS)) * 10) / 10.0 + " m/s"

Function Catmull#(p0#, p1#, p2#, p3#, t#)
	Return 0.5 * ((2 * p1) + (-p0 + p2) * t + (2 * p0 - 5 * p1 + 4 * p2 - p3) * t * t + (-p0 + 3 * p1 - 3 * p2 + p3) * t * t * t)
End Function

; Punkt bei Weglaenge s (in globale Variablen, Blitz kennt keine Tupel)
Global pk_x#, pk_z#
Dim rp_suche(0)
Function Punkt(s#)
	If s <= 0 Then pk_x = rp_x(0) : pk_z = rp_z(0) : Return
	If s >= rp_s(PROBEN) Then pk_x = rp_x(PROBEN) : pk_z = rp_z(PROBEN) : Return
	i = rp_suche(0)
	While i > 0 And rp_s(i) > s : i = i - 1 : Wend
	While i < PROBEN And rp_s(i + 1) < s : i = i + 1 : Wend
	rp_suche(0) = i
	f# = (s - rp_s(i)) / (rp_s(i + 1) - rp_s(i))
	pk_x = rp_x(i) + (rp_x(i + 1) - rp_x(i)) * f
	pk_z = rp_z(i) + (rp_z(i + 1) - rp_z(i)) * f
End Function

; Hoechster Boden (mit Baumkronen) in einem kleinen Umkreis
Function Boden#(x#, z#)
	h = MEER
	For dz = -6 To 6 Step 6
		For dx = -6 To 6 Step 6
			g = Gelaende_Hoehe(x + dx, z + dz)
			If g > h Then h = g
		Next
	Next
	Return h + 4
End Function

; ---- Flug ----------------------------------------------------------------

Spieler_Neu(rp_x(0), 100, rp_z(0))
CameraRange sp_kamera, 0.1, (SICHT + 1) * CH * 1.25
cy# = -1 : cg# = 0 : cn# = 0

For b = 0 To bilder - 1
	s# = laenge * b / Float(bilder - 1)
	Punkt(s)
	x# = pk_x : z# = pk_z

	; Hoehe: groesster Boden auf dem naechsten Stueck Weg, geglaettet
	soll_y# = Boden(x, z)
	For v = 10 To 40 Step 10
		Punkt(s + v) : bv# = Boden(pk_x, pk_z)
		If bv > soll_y Then soll_y = bv
	Next
	soll_y = soll_y + UEBER
	If cy < 0 Then cy = soll_y Else cy = cy + (soll_y - cy) * 0.035

	; Blick: voraus auf den Weg, leicht nach unten
	Punkt(s + VORAUS)
	dx# = pk_x - x : dz# = pk_z - z
	g# = ATan2(-dx, dz)
	boden_v# = Boden(pk_x, pk_z)
	n# = ATan2(cy - boden_v, VORAUS) * 0.6
	If n < 6 Then n = 6
	If n > 22 Then n = 22
	If b = 0
		cg = g : cn = n
	Else
		d# = g - cg
		While d > 180 : d = d - 360 : Wend
		While d < -180 : d = d + 360 : Wend
		cg = cg + d * 0.08
		cn = cn + (n - cn) * 0.05
	EndIf

	PositionEntity sp_kamera, x, cy, z
	RotateEntity sp_kamera, cn, cg, 0

	; alles in Sichtweite laden und bauen, dann zeichnen
	Laden_Takt(Int(Floor(x)) Sar 4, Int(Floor(z)) Sar 4, 100000)
	RenderWorld
	SaveBuffer BackBuffer(), ziel + "/f" + Right("0000" + b, 4) + ".bmp"
	Flip 0
	If (b Mod 30) = 0 Then Print "Bild " + b + " / " + bilder + "  Chunks " + welt_chunks
	If KeyHit(1) Then Exit
Next
Print "fertig: " + GraphicsWidth() + "x" + GraphicsHeight()
End

.route_daten
; Wegpunkte x, z (Saat 2026); 9999 beendet die Liste
Data -95, 340
Data -75, 210
Data -110, 120
Data -160, 40
Data -150, -60
Data -90, -120
Data -10, -150
Data 50, -175
Data 9999, 9999
