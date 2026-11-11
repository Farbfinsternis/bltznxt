; BLTZCRFT - Schnitte durch die Welt als Bild, zum Abstimmen der Hoehlen.
; Ohne Fenster: die Bilder schreibt das Programm selbst als BMP.
;
;   schnitt.exe [saat] [x0] [z0]
;
; schreibt
;   schnitt_senkrecht.bmp  1024 x 256: von x0 nach Osten bei z0, unten y = 0
;   schnitt_waagerecht.bmp 512 x 512 ab (x0, z0) auf Hoehe 70 (tief unter
;                          dem Meer, wo die grossen Gewoelbe liegen)
; und gibt aus, wie viel des Gesteins unter der Oberflaeche Hoehle ist.
;
; Farben: Gestein grau (Erz dunkler), Hoehle schwarz, Himmel hellblau,
; Wasser blau, Erde/Gras/Sand braun/gruen/gelb, Grundgestein dunkelgrau.

Include "../rauschen.bb"
Include "../bloecke.bb"
Include "../welt.bb"

Dim arg$(3)
a$ = Trim(CommandLine()) + " "
n = 0
While a <> "" And n < 3
	p = Instr(a, " ")
	arg(n) = Left(a, p - 1) : n = n + 1
	a = Trim(Mid(a, p + 1))
	If a <> "" Then a = a + " "
Wend
saat = 2026 : x0 = 0 : z0 = 0
If arg(0) <> "" Then saat = Int(arg(0))
If arg(1) <> "" Then x0 = Int(arg(1))
If arg(2) <> "" Then z0 = Int(arg(2))

Bloecke_Arten()
Welt_Saat(saat)

Global bmp_datei

; BMP mit 24 Bit, Zeilen von unten nach oben - Zeile 0 ist y = 0.
Function Bmp_Anfang(datei$, w, h)
	bmp_datei = WriteFile(datei)
	groesse = 54 + w * h * 3
	WriteByte bmp_datei, 66 : WriteByte bmp_datei, 77
	WriteInt bmp_datei, groesse : WriteInt bmp_datei, 0 : WriteInt bmp_datei, 54
	WriteInt bmp_datei, 40 : WriteInt bmp_datei, w : WriteInt bmp_datei, h
	WriteShort bmp_datei, 1 : WriteShort bmp_datei, 24
	WriteInt bmp_datei, 0 : WriteInt bmp_datei, w * h * 3
	WriteInt bmp_datei, 2835 : WriteInt bmp_datei, 2835
	WriteInt bmp_datei, 0 : WriteInt bmp_datei, 0
End Function

Function Bmp_Punkt(farbe)
	WriteByte bmp_datei, farbe And 255
	WriteByte bmp_datei, (farbe Shr 8) And 255
	WriteByte bmp_datei, (farbe Shr 16) And 255
End Function

Function Farbe(b, unter_tag)
	Select b
	Case B_LUFT
		If unter_tag Then Return $000000 Else Return $A8CCF0
	Case B_WASSER : Return $3060D0
	Case B_STEIN : Return $808080
	Case B_KOHLE : Return $404040
	Case B_EISEN : Return $B08870
	Case B_GRUND : Return $303030
	Case B_ERDE : Return $86603F
	Case B_GRAS : Return $5CA03A
	Case B_SAND, B_SANDSTEIN : Return $DCD0A0
	Case B_STAMM, B_LAUB : Return $2E6020
	End Select
	Return $A0A0A0
End Function

Function Chunks_Fuer(xa, za, xb, zb)
	For cz = (za Sar 4) - 1 To (zb Sar 4) + 1
		For cx = (xa Sar 4) - 1 To (xb Sar 4) + 1
			If Chunk_Holen(cx, cz) = Null Then Chunk_Neu(cx, cz)
		Next
	Next
End Function

Function Chunks_Weg()
	For c.Chunk = Each Chunk
		Chunk_Weg(c)
	Next
End Function

; Die Chunks liegen in einem Ring aus 32 x 32 Plaetzen (welt.bb) - mehr als
; 30 Chunks je Achse passen nicht gleichzeitig hinein. Darum entsteht jeder
; Schnitt in Abschnitten zu 256 Bloecken und wird erst in einer Bank
; gesammelt.
puffer = CreateBank(1024 * 256 * 2)

; senkrecht
t0 = MilliSecs() : n_chunks = 0
For abschnitt = 0 To 3
	xa = x0 + abschnitt * 256
	Chunks_Fuer(xa, z0, xa + 255, z0)
	n_chunks = n_chunks + welt_chunks
	For x = 0 To 255
		h = Gelaende_Hoehe(xa + x, z0)
		For y = 0 To 255
			PokeByte puffer, (y * 1024 + abschnitt * 256 + x) * 2, Welt_Block(xa + x, y, z0)
			PokeByte puffer, (y * 1024 + abschnitt * 256 + x) * 2 + 1, (y < h)
		Next
	Next
	Chunks_Weg()
Next
t_erzeugen = MilliSecs() - t0
gestein = 0 : hoehle = 0
Bmp_Anfang("schnitt_senkrecht.bmp", 1024, 256)
For y = 0 To 255
	For x = 0 To 1023
		b = PeekByte(puffer, (y * 1024 + x) * 2) : unter = PeekByte(puffer, (y * 1024 + x) * 2 + 1)
		Bmp_Punkt(Farbe(b, unter))
		If unter And y >= 5 And b <> B_WASSER
			gestein = gestein + 1
			If b = B_LUFT Then hoehle = hoehle + 1
		EndIf
	Next
Next
CloseFile bmp_datei
Print "senkrecht: " + n_chunks + " Chunks in " + t_erzeugen + " ms (" + (t_erzeugen / n_chunks) + " ms je Chunk samt Auslesen), Hoehle " + (hoehle * 100 / gestein) + " % des Gesteins"

; waagerecht auf y = 70, in vier Vierteln
For q = 0 To 3
	xa = x0 + (q And 1) * 256 : za = z0 + (q Shr 1) * 256
	Chunks_Fuer(xa, za, xa + 255, za + 255)
	For z = 0 To 255
		For x = 0 To 255
			PokeByte puffer, ((q Shr 1) * 256 + z) * 512 + (q And 1) * 256 + x, Welt_Block(xa + x, 70, za + z)
		Next
	Next
	Chunks_Weg()
Next
gestein = 0 : hoehle = 0
Bmp_Anfang("schnitt_waagerecht.bmp", 512, 512)
For z = 511 To 0 Step -1
	For x = 0 To 511
		b = PeekByte(puffer, z * 512 + x)
		Bmp_Punkt(Farbe(b, True))
		If b <> B_WASSER
			gestein = gestein + 1
			If b = B_LUFT Then hoehle = hoehle + 1
		EndIf
	Next
Next
CloseFile bmp_datei
Print "waagerecht y 70: Hoehle " + (hoehle * 100 / gestein) + " %"
End
