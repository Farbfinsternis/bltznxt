; BLTZCRFT - Abbauen ueber die Zeit
;
; Wie in Minecraft mit blosser Hand: die linke Maustaste halten, bis der Block
; bricht. Wie lange das dauert, steht als Haerte bei der Blockart
; (bl_haerte, Sekunden); Grundgestein bricht nie. Wer den Blick auf einen
; anderen Block richtet oder loslaesst, faengt von vorn an. Haelt man weiter,
; geht es nach einer kurzen Pause mit dem naechsten Block weiter.
;
; Den Fortschritt zeigen Risse auf dem Block, zehn Stufen. Die Risstexturen
; malt das Programm selbst: ein Muster aus verzweigten Linien, die vom
; Mittelpunkt aus wachsen; Stufe s zeigt die ersten s Zehntel davon. Die
; Texturen sind maskiert (Flag 4) - wo kein Riss ist, steht Alpha 0, dort
; bleibt der Block sichtbar.

Const AB_STUFEN = 10
Const AB_PAUSE_ZEIT# = 0.2  ; Sekunden zwischen zwei Bloecken beim Halten

Global ab_x, ab_y, ab_z, ab_aktiv
Global ab_anteil#           ; 0..1
Global ab_pause#
Global ab_wuerfel
Dim ab_tex(AB_STUFEN)

; ---- Ablauf (ohne Grafik) ------------------------------------------------

; Einen Bildtakt lang auf Block b bei x,y,z halten. Gibt True zurueck, wenn
; er in diesem Takt bricht.
Function Abbau_Halten(x, y, z, b, dt#)
	If ab_pause > 0
		ab_pause = ab_pause - dt
		Return False
	EndIf
	If Not (ab_aktiv And x = ab_x And y = ab_y And z = ab_z)
		ab_x = x : ab_y = y : ab_z = z : ab_aktiv = True
		ab_anteil = 0
	EndIf
	If bl_haerte(b) <= 0 Then Return False
	ab_anteil = ab_anteil + dt / bl_haerte(b)
	If ab_anteil < 0.9999 Then Return False   ; 6 x 0.1/0.6 ist knapp unter 1
	ab_aktiv = False : ab_anteil = 0 : ab_pause = AB_PAUSE_ZEIT
	Return True
End Function

; Losgelassen oder ins Leere geschaut: von vorn.
Function Abbau_Loslassen()
	ab_aktiv = False : ab_anteil = 0 : ab_pause = 0
End Function

Function Abbau_Stufe()
	If (Not ab_aktiv) Or ab_anteil <= 0 Then Return 0
	s = Floor(ab_anteil * AB_STUFEN) + 1
	If s > AB_STUFEN Then s = AB_STUFEN
	Return s
End Function

; ---- Risse ---------------------------------------------------------------

Dim ab_px(255), ab_py(255), ab_belegt(255)
Dim ast_x(15), ast_y(15), ast_haupt(15), ast_neben(15)
Dim richtung_x(3), richtung_y(3)
richtung_x(0) = 1 : richtung_x(1) = 0 : richtung_x(2) = -1 : richtung_x(3) = 0
richtung_y(0) = 0 : richtung_y(1) = 1 : richtung_y(2) = 0 : richtung_y(3) = -1

; Das Muster: vier Aeste laufen vom Mittelpunkt nach aussen und verzweigen
; sich. Ein Ast geht nur waagerecht oder senkrecht - meist in seine
; Hauptrichtung, manchmal quer dazu -, so entstehen zusammenhaengende
; Treppenlinien statt Punkten, die sich nur an den Ecken beruehren. Die
; Reihenfolge, in der die Texel dazukommen, ist die Reihenfolge der Stufen.
Function Riss_Muster()
	For k = 0 To 255 : ab_belegt(k) = False : Next
	n = 0
	aeste = 4
	For a = 0 To 3
		ast_x(a) = 7 + Rand(0, 1) : ast_y(a) = 7 + Rand(0, 1)
		ast_haupt(a) = a
		ast_neben(a) = (a + 1 + 2 * Rand(0, 1)) Mod 4    ; eine der beiden Querrichtungen
	Next
	For schritt = 1 To 24
		For a = 0 To aeste - 1
			x = ast_x(a) : y = ast_y(a)
			If x >= 0 And x < 16 And y >= 0 And y < 16 And n < 256
				k = x + y * 16
				If Not ab_belegt(k)
					ab_belegt(k) = True : ab_px(n) = x : ab_py(n) = y : n = n + 1
				EndIf
			EndIf
			If Rand(0, 2) = 0 Then r = ast_neben(a) Else r = ast_haupt(a)
			ast_x(a) = x + richtung_x(r) : ast_y(a) = y + richtung_y(r)
			; ab und zu zweigt ein Ast quer ab
			If aeste < 16 And Rand(0, 7) = 0
				ast_x(aeste) = x : ast_y(aeste) = y
				ast_haupt(aeste) = ast_neben(a)
				ast_neben(aeste) = (ast_neben(a) + 1 + 2 * Rand(0, 1)) Mod 4
				aeste = aeste + 1
			EndIf
		Next
	Next
	Return n
End Function

Function Abbau_Laden()
	SeedRnd 815
	n = Riss_Muster()
	g = 16 * TEX_ZOOM
	For s = 1 To AB_STUFEN
		ab_tex(s) = CreateTexture(g, g, 1 + 4)
		puffer = TextureBuffer(ab_tex(s))
		LockBuffer puffer
		; erst alles durchsichtig (Alpha 0), dann die Risse dieser Stufe
		For y = 0 To g - 1
			For x = 0 To g - 1
				WritePixelFast x, y, 0, puffer
			Next
		Next
		bis = n * s / AB_STUFEN
		For i = 0 To bis - 1
			d = 20 + (i Mod 3) * 12
			farbe = $FF000000 Or (d Shl 16) Or (d Shl 8) Or d
			For y = 0 To TEX_ZOOM - 1
				For x = 0 To TEX_ZOOM - 1
					WritePixelFast ab_px(i) * TEX_ZOOM + x, ab_py(i) * TEX_ZOOM + y, farbe, puffer
				Next
			Next
		Next
		UnlockBuffer puffer
	Next
	; knapp groesser als ein Block, damit die Risse vor seinen Flaechen liegen
	ab_wuerfel = CreateCube()
	ScaleEntity ab_wuerfel, 0.503, 0.503, 0.503
	EntityFX ab_wuerfel, 1
	HideEntity ab_wuerfel
End Function

Function Abbau_Zeigen()
	s = Abbau_Stufe()
	If s = 0
		HideEntity ab_wuerfel
	Else
		ShowEntity ab_wuerfel
		PositionEntity ab_wuerfel, ab_x + 0.5, ab_y + 0.5, ab_z + 0.5
		EntityTexture ab_wuerfel, ab_tex(s)
	EndIf
End Function
