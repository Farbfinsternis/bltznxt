; BUG-238 - Text-Texturen werden zwischengespeichert. Der Speicher darf nichts
; Falsches zeigen: gleicher Text in anderer Farbe, eine freigegebene und neu
; geladene Schrift (sie kann dieselbe Adresse bekommen) und ein neuer
; Grafikmodus (neuer Renderer, die alten Texturen sind weg).

Function Hell(rmin, gmin, bmin, rmax, gmax, bmax)
	; Gibt es im Streifen oben links ein Pixel in diesem Farbbereich?
	LockBuffer BackBuffer()
	For y = 0 To 39
		For x = 0 To 199
			c = ReadPixelFast(x, y) And $FFFFFF
			r = (c Shr 16) And 255 : g = (c Shr 8) And 255 : b = c And 255
			If r >= rmin And r <= rmax And g >= gmin And g <= gmax And b >= bmin And b <= bmax
				UnlockBuffer BackBuffer()
				Return True
			EndIf
		Next
	Next
	UnlockBuffer BackBuffer()
	Return False
End Function

Function Rechts()
	; rechteste Spalte mit einem hellen Pixel
	LockBuffer BackBuffer()
	rx = -1
	For y = 0 To 79
		For x = 0 To 299
			If (ReadPixelFast(x, y) And $FFFFFF) <> 0 And x > rx Then rx = x
		Next
	Next
	UnlockBuffer BackBuffer()
	Return rx
End Function

Graphics 300, 100, 32, 2
SetBuffer BackBuffer()
f1 = LoadFont("Arial", 14)
SetFont f1

Cls : Color 255, 255, 255 : Text 0, 0, "MMMM"
Print "weiss: " + Hell(200, 200, 200, 255, 255, 255)
Cls : Color 255, 0, 0 : Text 0, 0, "MMMM"
Print "rot: " + Hell(200, 0, 0, 255, 60, 60)
Print "kein weiss mehr: " + (Not Hell(200, 200, 200, 255, 255, 255))

; viele Bilder mit demselben und mit wechselndem Text
For i = 1 To 80
	Cls : Color 255, 255, 255
	Text 0, 0, "MMMM"
	Text 0, 20, "Bild " + i
	Flip 0
Next
Cls : Color 255, 255, 255 : Text 0, 0, "MMMM"
Print "nach 80 Bildern: " + Hell(200, 200, 200, 255, 255, 255)

; Schrift freigeben, eine groessere laden: derselbe Text muss breiter werden
Cls : Text 0, 0, "MMMM"
klein = Rechts()
FreeFont f1
f2 = LoadFont("Arial", 40)
SetFont f2
Cls : Text 0, 0, "MMMM"
Print "neue Schrift breiter: " + (Rechts() > klein + 20)

; neuer Grafikmodus: der Renderer wird neu angelegt
Graphics 300, 100, 32, 2
SetBuffer BackBuffer()
Cls : Color 0, 255, 0 : Text 0, 0, "MMMM"
Print "nach Graphics: " + Hell(0, 200, 0, 60, 255, 60)
End
