; BUG-131 - Graphics3D, Graphics und EndGraphics setzen den Zeichenzustand
; zurueck.
;
; Wie graphics() in bbruntime/bbgraphics.cpp und am Original gemessen
; (2026-09-17): Color wird weiss, ClsColor schwarz, die Schrift die
; Standardschrift, Origin und Viewport fallen weg, und gezeichnet wird in den
; Puffer des Modus - auch wenn vorher ein ImageBuffer aktiv war.
;
; Die Schrift wird nur mit der beim Start verglichen: ihre Hoehe selbst weicht
; noch ab (BUG-139).

Graphics3D 320, 240, 0, 2
Global f0 = FontHeight()
Global fnt = LoadFont("Arial", 40)

Function Verstellen()
	Color 0, 0, 255
	ClsColor 200, 100, 50
	Origin 30, 20
	Viewport 10, 10, 50, 50
	SetFont fnt
End Function

Function Pruefen$(was$)
	s$ = was + ": color " + ColorRed() + "," + ColorGreen() + "," + ColorBlue()
	s = s + " schrift wie beim start " + (FontHeight() = f0)
	Cls
	s = s + " cls " + (ReadPixel(100, 100) And $FFFFFF)
	Color 255, 255, 255
	Rect 0, 0, 5, 5, 1
	s = s + " origin/viewport " + (ReadPixel(2, 2) And $FFFFFF) + " " + (ReadPixel(32, 22) And $FFFFFF)
	Return s
End Function

Verstellen()
Graphics3D 320, 240, 0, 2
fnt = LoadFont("Arial", 40)
Print Pruefen("graphics3d")

Verstellen()
Graphics 320, 240, 0, 2
fnt = LoadFont("Arial", 40)
Print Pruefen("graphics")

Verstellen()
EndGraphics
Graphics3D 320, 240, 0, 2
fnt = LoadFont("Arial", 40)
Print Pruefen("endgraphics")

; ein vorher aktiver ImageBuffer gilt nach dem Moduswechsel nicht mehr
img = CreateImage(8, 8)
SetBuffer ImageBuffer(img)
Graphics3D 320, 240, 0, 2
Cls
WritePixel 5, 5, $00FF00
Print "writepixel im backbuffer " + (ReadPixel(5, 5, BackBuffer()) And $FFFFFF)

Print "fertig"
End
