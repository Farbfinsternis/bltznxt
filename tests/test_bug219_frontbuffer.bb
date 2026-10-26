; BUG-219 - Zeichnen in den Frontpuffer erscheint ohne Flip.
;
; Im Fenster sind Front- und Backbuffer im Original dieselbe Flaeche; was ueber
; den Frontpuffer gezeichnet wird, zeigt das Fenster nach hoechstens 100 ms,
; sobald das Programm Nachrichten abholt (Delay, WaitKey ...). BLTZNXT zeigt
; die Flaeche dabei an und muss ihren Inhalt behalten: nach jedem Delay liest
; der Test die Pixel zurueck. Das Bild selbst prueft build/bug219_20261026/fb.
; Erwartung: Ausgabe von Blitz3D 11.8.
Graphics 320,240,0,2
Color 255,0,0 : Rect 10,10,40,40
Delay 250
Print Hex(ReadPixel(30,30,FrontBuffer()) And $FFFFFF)

SetBuffer BackBuffer()
Color 0,255,0 : Rect 60,10,40,40
Delay 250
Print Hex(ReadPixel(80,30,BackBuffer()) And $FFFFFF)
Print Hex(ReadPixel(30,30,BackBuffer()) And $FFFFFF)

SetBuffer FrontBuffer()
Color 255,255,255 : Text 0,100,"Hallo"
Delay 250
Print Hex(ReadPixel(80,30,FrontBuffer()) And $FFFFFF)
Print Hex(ReadPixel(30,30,FrontBuffer()) And $FFFFFF)

; 3D: auch hier bleibt die Flaeche erhalten
Graphics3D 320,240,0,2
SetBuffer FrontBuffer()
Color 0,0,255 : Rect 10,10,40,40
Delay 250
Print Hex(ReadPixel(30,30,FrontBuffer()) And $FFFFFF)
Print "DONE"
