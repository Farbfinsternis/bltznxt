; 3D-Klang: CreateListener und EmitSound, dazu Panorama im Mischer.
;
; Der Test hoert selbst zu: mit SDL3s Audiotreiber "disk" landet alles, was
; der Mischer ausgibt, in einer Datei (float, Stereo). Der Klang
; tests/assets/klang_gleich.wav ist ein gleichbleibendes Signal der Hoehe
; 0.5, eine Viertelsekunde lang - so steht in jedem Abschnitt der Datei
; direkt, wie laut er links und rechts ankam. Gemeldet wird je Abschnitt der
; Mittelwert der mittleren Haelfte in Tausendsteln.
;
; SDL haelt die Datei exklusiv offen, bis das Audiogeraet schliesst, und
; dafuer gibt es keinen Befehl. Deshalb startet der Test sich selbst als
; zweiten Prozess ("spielen"), der die Klaenge abspielt und endet; der erste
; wartet, bis er die Datei lesen kann, und wertet sie aus.
;
; Erwartet nach FMOD 3 (bb_listener.h): Lautstaerke 1 / (1 + rolloff * (d - 1))
; ab einem Meter, Panorama = Anteil der Richtung auf der Rechts-Achse des
; Listeners, als Balance: die Gegenseite wird leiser, die eigene bleibt.

Const DATEI$ = "bin/test_3d_klang.raw"
Dim wl#(200000), wr#(200000)
If CommandLine() <> "spielen"
	DeleteFile DATEI
	ExecFile "bin\test_3d_klang.exe spielen"
	t = MilliSecs()
	Repeat
		f = ReadFile(DATEI)
		If f = 0 Then Delay 50
	Until f <> 0 Or MilliSecs() - t > 20000
	Auswerten(f)
	End
EndIf

SetEnv "SDL_AUDIO_DRIVER", "disk"
SetEnv "SDL_AUDIO_DISK_OUTPUT_FILE", DATEI

Graphics3D 320,240,0,2
cam = CreateCamera()
lis = CreateListener(cam)
snd = Load3DSound("tests/assets/klang_gleich.wav")
flach = LoadSound("tests/assets/klang_gleich.wav")
p = CreatePivot()
; Was nur der spielende Prozess sieht, geht in die Datei selbst nicht ein;
; Fehler hier enden mit einer Laufzeitmeldung und leerer Auswertung.
If EntityClass(lis) <> "Listener" Or GetParent(lis) <> cam Then RuntimeError "Listener falsch"

Global schritt = 0
; Spielen bis zum Ende, dann etwas Stille als Trenner.
Function Hoeren(ch, bewegen = 0)
	t = MilliSecs()
	While ChannelPlaying(ch) And MilliSecs() - t < 3000
		If bewegen Then MoveEntity bewegen, 0, 0, -1
		UpdateWorld : RenderWorld
		Delay 5
	Wend
	t = MilliSecs()
	While MilliSecs() - t < 60
		UpdateWorld : RenderWorld
		Delay 5
	Wend
	schritt = schritt + 1
End Function

; 1) rechts, 5 m: 0.5 * 1/5 nur rechts
PositionEntity p, 5, 0, 0 : Hoeren(EmitSound(snd, p))
; 2) vorn, 2 m: halb, beide Seiten
PositionEntity p, 0, 0, 2 : Hoeren(EmitSound(snd, p))
; 3) links, 3 m
PositionEntity p, -3, 0, 0 : Hoeren(EmitSound(snd, p))
; 4) naeher als 1 m rechts: voll laut, das Panorama laeuft zur Mitte aus
PositionEntity p, 0.5, 0, 0 : Hoeren(EmitSound(snd, p))
; 5) schraeg rechts vorn, 2.83 m
PositionEntity p, 2, 0, 2 : Hoeren(EmitSound(snd, p))
; 6) der Listener dreht sich nach rechts: die Quelle rechts ist jetzt vorn
RotateEntity cam, 0, -90, 0
PositionEntity p, 5, 0, 0 : Hoeren(EmitSound(snd, p))
RotateEntity cam, 0, 0, 0
; 7) PlaySound spielt auch einen 3D-Klang flach
Hoeren(PlaySound(snd))
; 8) EmitSound mit einem Klang aus LoadSound: flach, wie in FMOD (FSOUND_2D)
PositionEntity p, 100, 0, 0 : Hoeren(EmitSound(flach, p))
; 9) ChannelPan im Mischer: ganz nach links
ch = PlaySound(flach) : ChannelPan ch, -1 : Hoeren(ch)
; 10) ein neuer Listener mit rolloff 0.5: vorn 5 m ist 1/3
FreeEntity lis
lis = CreateListener(cam, 0.5)
PositionEntity p, 0, 0, 5 : Hoeren(EmitSound(snd, p))
; 11) Doppler: die Quelle kommt je UpdateWorld 1 m naeher, doppler 100 -
;     Tonhoehe 340 / (340 - 100) = 1.42, der Klang ist kuerzer
FreeEntity lis
lis = CreateListener(cam, 1, 100)
PositionEntity p, 0, 0, 1000 : Hoeren(EmitSound(snd, p), p)
; 12) die Quelle folgt ihrer Entity: sie springt waehrend des Klangs nach links
FreeEntity lis
lis = CreateListener(cam)
PositionEntity p, 2, 0, 0
ch = EmitSound(snd, p)
t = MilliSecs()
While ChannelPlaying(ch)
	If MilliSecs() - t > 120 Then PositionEntity p, -2, 0, 0
	UpdateWorld : RenderWorld
	Delay 5
Wend
Hoeren(0)
; 13) LoopSound spielt nichts, stellt den Klang aber auf Wiederholung: der
;     Kanal von EmitSound laeuft, bis StopChannel ihn nach 0.6 s anhaelt
schleife = Load3DSound("tests/assets/klang_gleich.wav")
LoopSound schleife
PositionEntity p, 0, 0, 1
ch = EmitSound(schleife, p)
t = MilliSecs()
While MilliSecs() - t < 600
	UpdateWorld : RenderWorld
	Delay 5
Wend
StopChannel ch
Hoeren(0)
End

; Die Datei lesen: Abschnitte ohne Stille dazwischen
Function Auswerten(f)
	If f = 0 Then Print "datei nicht lesbar" : Return
	n = 0 : nr = 0
	While Not Eof(f)
		l# = ReadFloat(f) : r# = ReadFloat(f)
		If Abs(l) > 0.0000001 Or Abs(r) > 0.0000001
			If n < 200000 Then wl(n) = l : wr(n) = r : n = n + 1
		ElseIf n > 0
			nr = nr + 1
			Abschnitt(nr, n)
			n = 0
		EndIf
	Wend
	If n > 0 Then nr = nr + 1 : Abschnitt(nr, n)
	CloseFile f
	Print "abschnitte " + nr
End Function

Function Abschnitt(nr, n)
	a = n / 4 : b = n - n / 4
	sl# = 0 : sr# = 0
	For i = a To b - 1
		sl = sl + wl(i) : sr = sr + wr(i)
	Next
	If nr = 11
		; Doppler: Laenge statt Lautstaerke; 0.25 s sind 11025 Bilder bei 44.1 kHz
		Print nr + ": laenge 1/" + Int(11025.0 / n * 100 + 0.5) / 100.0 + " der urspruenglichen"
	ElseIf nr = 13
		Print nr + ": schleife ueber zwei durchlaeufe " + (n > 2 * 11025) + ", links " + T(sl / (b - a)) + " rechts " + T(sr / (b - a))
	ElseIf nr = 12
		; der erste Teil rechts, der letzte links
		Print nr + ": anfang " + T(wl(n / 8)) + " / " + T(wr(n / 8)) + ", ende " + T(wl(n - n / 8)) + " / " + T(wr(n - n / 8))
	Else
		Print nr + ": links " + T(sl / (b - a)) + " rechts " + T(sr / (b - a))
	EndIf
End Function

Function T(x#)
	Return Int(Floor(x * 1000 + 0.5))
End Function
