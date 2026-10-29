; BUG-227 - LoadFont mit einem Dateinamen nimmt die installierte Schrift.
;
; gxGraphics::loadFont im Original: steht ein Punkt im Namen, versucht es
; AddFontResource mit der Datei und nimmt dann den Namen ohne Ordner und ohne
; Endung (ab dem ersten Punkt) als Familie. LoadFont("verdana.ttf",12) ohne
; die Datei neben dem Programm ist also Verdana - ein verbreitetes Muster in
; altem Code (15 Aufrufe in github.com/Pakz001/Blitz3d). Eine unbekannte
; Familie ersetzt GDI durch eine Schrift mit den Massen von Arial. Bei uns
; ergab beides die eingebaute 8x8-Schrift.
;
; Erwartet sind FontHeight und FontWidth des Originals (gemessen 2026-10-29).
; StringWidth weicht um wenige Pixel ab, weil wir nicht ueber GDI zeichnen,
; und ist deshalb nicht Teil des Tests.
Graphics 320,240,0,2
Probe("verdana.ttf", 16, 0)
Probe("Verdana.ttf", 32, 1)
Probe("fonts\arial.ttf", 16, 0)
Probe("gibtsnicht", 16, 0)
Probe("gibtsnicht", 24, 1)
Probe("courier.fon", 12, 0)
Probe("Times New Roman.ttf", 20, 0)
Probe("tahoma.TTF", 14, 0)
Probe("arial.ttf.bak", 16, 0)
End

Function Probe(n$, h, b)
	f = LoadFont(n, h, b) : SetFont f
	Print n + " " + h + " " + b + ": " + FontHeight() + " " + FontWidth()
	SetFont 0
	FreeFont f
End Function
