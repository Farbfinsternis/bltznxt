; BLTZCRFT - Karte der Welt von oben, zum Abstimmen des Gelaendes.
; Schreibt karte.bmp (2 Bloecke je Bildpunkt) und beendet sich.
;
;   blitzcc werkzeug/karte.bb -o karte  ;  karte.exe [saat] [massstab]

Include "../rauschen.bb"
Include "../bloecke.bb"
Include "../welt.bb"

Graphics 800, 800, 0, 2
SetBuffer BackBuffer()

saat = 1 : mass = 2
a$ = CommandLine()
If a <> ""
	saat = Int(a)
	If Instr(a, " ") Then mass = Int(Mid(a, Instr(a, " ") + 1))
EndIf
Welt_Saat(saat)

LockBuffer BackBuffer()
For py = 0 To 799
	For px = 0 To 799
		x = (px - 400) * mass : z = (400 - py) * mass
		h = Gelaende_Hoehe(x, z)
		hx = Gelaende_Hoehe(x + 1, z)
		steil = Abs(hx - h)
		o = Oberflaeche(x, z, h, steil)
		Select o
		Case B_GRAS : r = 90 : g = 150 : b = 50
		Case B_SAND : r = 220 : g = 205 : b = 160
		Case B_STEIN : r = 125 : g = 125 : b = 125
		Case B_SCHNEE : r = 245 : g = 245 : b = 250
		Case B_KIES : r = 130 : g = 120 : b = 110
		Case B_TON : r = 160 : g = 165 : b = 180
		Default : r = 255 : g = 0 : b = 255
		End Select
		licht# = 1 + (hx - h) * 0.12
		If h < MEER
			t = MEER - h
			r = 30 + 40 * (1 - t / 30.0) : g = 70 + 60 * (1 - t / 30.0) : b = 190 - t
		Else
			r = r * licht : g = g * licht : b = b * licht
		EndIf
		If r < 0 Then r = 0
		If g < 0 Then g = 0
		If b < 0 Then b = 0
		If r > 255 Then r = 255
		If g > 255 Then g = 255
		If b > 255 Then b = 255
		WritePixelFast px, py, (r Shl 16) Or (g Shl 8) Or b
	Next
Next
UnlockBuffer BackBuffer()
SaveBuffer BackBuffer(), "karte.bmp"
End
