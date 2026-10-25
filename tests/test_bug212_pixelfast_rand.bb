; BUG-212 - WritePixelFast/ReadPixelFast hinter dem Rand.
;
; Das Original prueft bei den Fast-Befehlen nichts und rechnet linear in die
; gesperrte Flaeche: x = Breite landet in der naechsten Zeile (gemessen an
; Blitz3D 11.8 mit 64x64-Bild und Backbuffer, build/bug212_20261025/edge -
; die Werte unten sind die des Originals). Bei uns schrieb ein Zugriff ganz
; hinter den Puffer in den Heap: Games/wing_ring faerbt sein Logo mit
; "For x=0 To ImageWidth(logo) : For y=0 To ImageHeight(logo)" und stuerzte
; beim naechsten LockBuffer mit 0xC0000374 ab. Jetzt wird so ein Zugriff
; verworfen, Lesen liefert 0.
Graphics 640,480,0,2
img=CreateImage(64,64)
SetBuffer ImageBuffer(img)
LockBuffer
For y=0 To 63
	For x=0 To 63
		WritePixelFast x,y,$102030
	Next
Next
WritePixelFast 64,0,$FF0000
UnlockBuffer
LockBuffer
; wie im Original: hinter dem Zeilenende geht es in der naechsten Zeile weiter
Print ReadPixelFast(0,1) And $FFFFFF
Print ReadPixelFast(64,0) And $FFFFFF
Print ReadPixelFast(1,1) And $FFFFFF
Print ReadPixelFast(63,0) And $FFFFFF
UnlockBuffer

; der Absturzweg aus wing_ring, dreimal mit neuem Sperren dazwischen
For n=1 To 3
	LockBuffer
	For x=0 To ImageWidth(img)
		For y=0 To ImageHeight(img)
			WritePixelFast x,y,$00FF00
		Next
	Next
	UnlockBuffer
Next

; ganz ausserhalb des Puffers: nichts geschrieben, gelesen 0
LockBuffer
WritePixelFast -1,0,$FFFFFF
WritePixelFast 0,64,$FFFFFF
WritePixelFast 100000,100000,$FFFFFF
WritePixelFast -100000,-100000,$FFFFFF
Print ReadPixelFast(-1,0)
Print ReadPixelFast(0,64)
Print ReadPixelFast(100000,100000)
Print ReadPixelFast(63,63) And $FFFFFF
CopyPixelFast 0,0,ImageBuffer(img),64,63,ImageBuffer(img)
CopyPixelFast 64,63,ImageBuffer(img),0,0,ImageBuffer(img)
Print ReadPixelFast(0,0) And $FFFFFF
UnlockBuffer
FreeImage img
img=CreateImage(64,64)
Print "ok"
