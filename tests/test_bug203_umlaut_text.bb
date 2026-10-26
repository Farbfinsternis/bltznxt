; BUG-203 - Umlaute in TTF-Text werden als ANSI gelesen, nicht als UTF-8.
;
; Ein Blitz-String ist ANSI (diese Datei ist Windows-1252); SDL_ttf bekam die
; Bytes als UTF-8 und mass fuer "ה" das Ersatzzeichen: StringWidth("ה") war 15
; bei StringWidth("a") 11, im Original sind beide 9 (gemessen,
; build/bug203_20261025/sw). Die absolute Breite haengt an BUG-139 (Schrift-
; groesse), geprueft wird deshalb das Verhaeltnis. Fenstertitel und Konsole
; sind von aussen geprueft (title/, console/), aus einem Programm nicht lesbar.
Graphics 200,150,0,2
f = LoadFont("Arial", 20)
SetFont f
Print StringWidth("ה") = StringWidth("a")
Print StringWidth("ײ") = StringWidth("O")
Print StringWidth("הצüß") < StringWidth("aouss")
Print Len("הצü")
