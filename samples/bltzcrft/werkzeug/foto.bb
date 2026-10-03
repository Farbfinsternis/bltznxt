; BLTZCRFT - ein Bild der Welt von einer festen Stelle, ohne Maus und
; Fenster-Fokus. Misst nebenbei, wie lange Erzeugen und Bauen dauern.
;
;   foto.exe saat x z gier neig [hoehe ueber Boden | absolute y wenn > 200-]
;   schreibt foto.bmp und gibt die Zeiten aus.

Const SICHT = 12
Include "../rauschen.bb"
Include "../bloecke.bb"
Include "../welt.bb"
Include "../netz.bb"
Include "../spieler.bb"
Include "../laden.bb"

Graphics3D 1280, 720, 0, 2
SetBuffer BackBuffer()

Dim arg$(7)
a$ = Trim(CommandLine()) + " "
n = 0
While a <> "" And n < 7
	p = Instr(a, " ")
	arg(n) = Left(a, p - 1) : n = n + 1
	a = Trim(Mid(a, p + 1))
	If a <> "" Then a = a + " "
Wend
saat = Int(arg(0)) : x = Int(arg(1)) : z = Int(arg(2))
gier# = Float(arg(3)) : neig# = Float(arg(4))
ueber# = 1.62
If arg(5) <> "" Then ueber = Float(arg(5))

Bloecke_Laden()
Netz_Laden()
Welt_Saat(saat)
Spieler_Neu(x + 0.5, 100, z + 0.5)

t0 = MilliSecs()
cx = x Sar 4 : cz = z Sar 4
For dz = -SICHT - 1 To SICHT + 1
	For dx = -SICHT - 1 To SICHT + 1
		Chunk_Neu(cx + dx, cz + dz)
	Next
Next
t1 = MilliSecs()
flaechen = 0
For dz = -SICHT To SICHT
	For dx = -SICHT To SICHT
		Chunk_Bauen(Chunk_Holen(cx + dx, cz + dz))
		flaechen = flaechen + nz_bloecke
	Next
Next
t2 = MilliSecs()
n_erz = (2 * SICHT + 3) * (2 * SICHT + 3)
n_bau = (2 * SICHT + 1) * (2 * SICHT + 1)
Print "erzeugt " + n_erz + " Chunks in " + (t1 - t0) + " ms (" + Float(t1 - t0) / n_erz + " ms je Chunk)"
Print "gebaut  " + n_bau + " Chunks in " + (t2 - t1) + " ms (" + Float(t2 - t1) / n_bau + " ms je Chunk), " + flaechen + " Flaechen"

Local y#
If ueber > 200
	y# = ueber - 1000
Else
	y = HOEHE - 1
	While y > 0 And (Not bl_fest(Welt_Block(x, y, z)))
		y = y - 1
	Wend
	y = y + 1 + ueber - SP_AUGE
EndIf
Spieler_Setzen(x + 0.5, y, z + 0.5)
sp_gier = gier : sp_neig = neig
Spieler_Kamera(1)

t3 = MilliSecs()
RenderWorld
t4 = MilliSecs()
RenderWorld
t5 = MilliSecs()
For i = 1 To 20 : RenderWorld : Next
t6 = MilliSecs()
Print "erstes Bild " + (t4 - t3) + " ms, zweites " + (t5 - t4) + " ms, danach " + (t6 - t5) / 20.0 + " ms"

; ein Chunk neu gebaut: wie teuer ist das naechste Bild?
c.Chunk = Chunk_Holen(cx, cz)
t7 = MilliSecs()
Chunk_Bauen(c)
t8 = MilliSecs()
RenderWorld
t9 = MilliSecs()
Print "ein Chunk neu: bauen " + (t8 - t7) + " ms, Bild danach " + (t9 - t8) + " ms"

RenderWorld
Color 255, 255, 255
Text 10, 10, "Saat " + saat + "  x " + x + "  y " + Int(sp_y) + "  z " + z + "  gier " + gier + "  neig " + neig
SaveBuffer BackBuffer(), "foto.bmp"
End
