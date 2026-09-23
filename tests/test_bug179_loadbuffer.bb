; BUG-179 - LoadBuffer skaliert die Datei auf den Puffer, ohne Sperre.
;
; Wie bbLoadBuffer (bbgraphics.cpp): laden, mit tformCanvas bilinear auf die
; Groesse des Puffers bringen (Nachbarn ausserhalb der Datei zaehlen als
; Schwarz) und deckend bei 0,0 hineinkopieren. Der Origin des Puffers wird
; 0,0 und bleibt es, sein Viewport beschneidet die Kopie. Auf einen
; gesperrten Puffer liefert es 1, aendert aber nichts. Vorher ging es bei uns
; nur gesperrt und uebernahm die Groesse der Datei.
;
; Gemessen am Original (2026-09-23, build/load20260923): die Dateien dort
; sind byteweise gleich; der 10->37-Fall mit zwei Gleichstaenden auf .5 ist
; hier bewusst nicht dabei.

Graphics3D 320,240,0,2
SetBuffer BackBuffer()

Function Summe$(buf, w, h)
	s = 0
	For y = 0 To h - 1
		For x = 0 To w - 1
			s = (s * 31 + (ReadPixel(x, y, buf) And $FFFFFF)) And $FFFFFF
		Next
	Next
	Return s
End Function

; 4x3 in ein Bild 37x21: vergroessert, Groesse bleibt
img = CreateImage(37,21)
Print "laden: " + LoadBuffer(ImageBuffer(img),"tests/assets/farben4x3.bmp")
Print "groesse: " + ImageWidth(img) + "x" + ImageHeight(img)
Print "ecke: " + Hex(ReadPixel(0,0,ImageBuffer(img))) + " mitte: " + Hex(ReadPixel(18,10,ImageBuffer(img)))
Print "summe: " + Summe(ImageBuffer(img),37,21)

; in eine Textur 2x2: verkleinert
; (Laden und Pruefen in getrennten Anweisungen: in einem Ausdruck waehlt das
; Original die Reihenfolge nach Registerbedarf und prueft vor dem Laden, BUG-29.)
tex = CreateTexture(2,2)
Print "textur: " + LoadBuffer(TextureBuffer(tex),"tests/assets/farben4x3.bmp")
Print "textur summe: " + Summe(TextureBuffer(tex),2,2)

; Origin wird 0,0, der Viewport beschneidet
ClsColor 0,0,0 : Cls
Origin 10,10
Viewport 20,20,40,30
Print "bildschirm: " + LoadBuffer(BackBuffer(),"tests/assets/farben4x3.bmp")
Viewport 0,0,320,240
Color 255,255,255 : Plot 0,0
Print "origin: " + Hex(ReadPixel(0,0)) + " " + Hex(ReadPixel(10,10))
Print "viewport: " + Hex(ReadPixel(19,19)) + " " + Hex(ReadPixel(20,20)) + " " + Hex(ReadPixel(59,49)) + " " + Hex(ReadPixel(60,50))

; gesperrt: 1, aber nichts aendert sich
img2 = CreateImage(4,3)
LockBuffer ImageBuffer(img2)
Print "gesperrt: " + LoadBuffer(ImageBuffer(img2),"tests/assets/farben4x3.bmp") + " " + Hex(ReadPixelFast(1,0,ImageBuffer(img2)))
UnlockBuffer ImageBuffer(img2)
Print "danach: " + Hex(ReadPixel(1,0,ImageBuffer(img2)))

Print "fehlt: " + LoadBuffer(BackBuffer(),"tests/assets/gibtsnicht.bmp")
End
