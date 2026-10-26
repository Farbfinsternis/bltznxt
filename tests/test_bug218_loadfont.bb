; BUG-218 / BUG-139 - LoadFont findet Schriften ueber ihren Namen und haelt die Hoehe.
;
; Das Original fragt GDI nach dem Familiennamen und gibt die Hoehe als
; Zellenhoehe an: LoadFont("arial black",48) misst 48, die Vorgabehoehe ist
; 12 (gemessen an Blitz3D 11.8, build/bug218_20261025/fonts). Bei uns fanden
; nur Schriften, deren Datei wie die Schrift heisst (arial.ttf), alle anderen
; fielen still auf die eingebaute 8-Pixel-Schrift; und die Hoehe war die
; Punktgroesse von SDL_ttf (Arial 40 mass 45). Toleranz 1 Pixel fuer die
; Rundung von GDI (Arial 20 misst dort 19).
Graphics 320,240,0,2
Print Nah("Arial", 40)
Print Nah("arial black", 48)
Print Nah("Times New Roman", 24)
Print Nah("Courier New", 16)
Print Nah("courier", 16)
Print Nah("verdana", 16)
f = LoadFont("Arial") : SetFont f
Print Abs(FontHeight() - 12) <= 1
; fett ist breiter als normal
f = LoadFont("Arial", 20) : SetFont f : w1 = StringWidth("Hello World")
f = LoadFont("Arial", 20, 1) : SetFont f : w2 = StringWidth("Hello World")
Print w2 > w1

Function Nah(n$, h)
	f = LoadFont(n$, h)
	SetFont f
	r = Abs(FontHeight() - h) <= 1
	FreeFont f
	Return r
End Function
