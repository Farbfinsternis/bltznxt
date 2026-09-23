; BUG-167 - SaveBuffer und SaveImage schreiben immer eine 24-Bit-BMP.
;
; Im Original gehen beide ueber saveCanvas (bbgraphics.cpp): unkomprimierte
; BMP mit 54 Byte Kopf, Zeilen von unten, je Zeile auf 4 Byte aufgefuellt -
; auch wenn der Name auf ".png" endet. Der Puffer muss nicht gesperrt sein.
; Vorher schrieben wir nur aus einem gesperrten Puffer und dann PNG.
;
; Die Dateien werden zurueckgelesen: Groesse, Kopffelder und eine Summe ueber
; alle Bytes. Gemessen am Original (2026-09-23, build/save20260923): dort
; sind die Dateien byteweise gleich.

Graphics3D 320,240,0,2

Function Pruefe(name$)
	f = ReadFile(name$)
	If f = 0 Then Print name$ + ": fehlt" : Return
	n = FileSize(name$)
	b1 = ReadByte(f) : b2 = ReadByte(f)
	groesse = ReadInt(f) : ReadInt(f) : offset = ReadInt(f)
	kopf = ReadInt(f) : breite = ReadInt(f) : hoehe = ReadInt(f)
	ebenen = ReadShort(f) : bits = ReadShort(f)
	SeekFile f,0
	summe = 0
	For i = 1 To n
		summe = (summe * 31 + ReadByte(f)) And $FFFFFF
	Next
	CloseFile f
	Print name$ + ": " + n + " " + Chr(b1) + Chr(b2) + " " + groesse + " " + offset + " " + kopf + " " + breite + "x" + hoehe + " " + ebenen + " " + bits + " summe " + summe
	DeleteFile name$
End Function

SetBuffer BackBuffer()
ClsColor 10,20,30 : Cls
Color 255,0,0 : Rect 10,10,50,40
Color 1,2,3 : Plot 319,239
Color 250,128,7 : Plot 0,0
Print "ungesperrt: " + SaveBuffer(BackBuffer(),"__bug167_a.png")
Pruefe("__bug167_a.png")

LockBuffer BackBuffer()
WritePixelFast 5,5,$FF00FF
Print "gesperrt: " + SaveBuffer(BackBuffer(),"__bug167_b.bmp")
UnlockBuffer BackBuffer()
Pruefe("__bug167_b.bmp")

img = CreateImage(37,21)
SetBuffer ImageBuffer(img)
ClsColor 200,100,50 : Cls
Color 0,0,0 : Rect 3,4,10,5
Color 255,255,255 : Plot 36,20
SetBuffer BackBuffer()
Print "saveimage: " + SaveImage(img,"__bug167_c.bmp")
Pruefe("__bug167_c.bmp")

Print "falscher pfad: " + SaveBuffer(BackBuffer(),"__gibtsnicht\x.bmp")
End
