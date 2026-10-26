; BUG-139 - FontWidth und FontHeight wie GDI im Original.
;
; FontWidth ist im Original tmMaxCharWidth (die groesste Zeichenbreite der
; Schrift nach dem Hinting), FontHeight tmHeight. BLTZNXT meldete die Breite
; von "M" (Arial 12: 8 statt 24). Nur Schriften, die jedes Windows hat.
; Erwartung: Ausgabe von Blitz3D 11.8.
Graphics 320,240,0,2
Restore schriften
Repeat
	Read n$,g,b
	If n$="" Then Exit
	f=LoadFont(n$,g,b) : SetFont f
	Print n$+" "+g+" "+b+": "+FontWidth()+" "+FontHeight()
	FreeFont f
Forever
Print "DONE"
End
.schriften
Data "arial",12,0, "arial",40,0, "arial",20,1, "times new roman",20,0, "courier new",16,0, "verdana",18,0, "tahoma",14,0, "",0,0
