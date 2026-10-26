; BUG-195: OpenMovie, DrawMovie, CloseMovie, MovieWidth, MovieHeight, MoviePlaying
; Testfilm: 64x48, 10 Bilder zu 100 ms, obere Haelfte Farbe je Bild, unten weiss
; (build/movie_20261026/mkavi.py). Werte gemessen am Original 2026-10-26.
Graphics 320,240,0,2
SetBuffer BackBuffer()
m=OpenMovie("tests\assets\gibtsnicht.avi")
Print "fehlt " + m
m=OpenMovie("tests\assets\bug195_film.avi")
Print "w " + MovieWidth(m) + " h " + MovieHeight(m) + " p " + MoviePlaying(m)
t0=MilliSecs()
tl=t0
n=0
Repeat
	Cls
	p=DrawMovie(m,0,0)
	t=MilliSecs()
	a=ReadPixel(5,5) And $FFFFFF
	b=ReadPixel(5,40) And $FFFFFF
	c=ReadPixel(63,47) And $FFFFFF
	d=ReadPixel(64,48) And $FFFFFF
	takt$="-"
	If n>=1 And n<=9 Then
		If t-tl>=60 And t-tl<=160 Then takt$="takt ok" Else takt$="takt " + (t-tl)
	EndIf
	Print n + " " + takt$ + " p " + p + " " + Hex(a) + " " + Hex(b) + " " + Hex(c) + " " + Hex(d)
	tl=t
	n=n+1
	Flip 0
Until p=0 Or n>60
ms=MilliSecs()-t0
If ms>=800 And ms<=1100 Then Print "ende n " + n + " dauer ok p " + MoviePlaying(m) Else Print "ende n " + n + " ms " + ms
p=DrawMovie(m,0,0)
Print "nach ende " + p
CloseMovie m

m2=OpenMovie("tests\assets\bug195_film.avi")
Cls
p=DrawMovie(m2,100,10,128,96)
Print "skaliert p " + p
Print Hex(ReadPixel(105,15) And $FFFFFF) + " " + Hex(ReadPixel(227,105) And $FFFFFF) + " " + Hex(ReadPixel(227,50) And $FFFFFF) + " " + Hex(ReadPixel(228,106) And $FFFFFF) + " " + Hex(ReadPixel(99,9) And $FFFFFF)
Delay 300
p=DrawMovie(m2,0,0,-1,-1)
Print "nach 300 " + p + " " + Hex(ReadPixel(5,5) And $FFFFFF)

img=CreateImage(80,60)
SetBuffer ImageBuffer(img)
p=DrawMovie(m2,10,10,32,24)
Print "image p " + p
Print Hex(ReadPixel(10,10) And $FFFFFF) + " " + Hex(ReadPixel(41,21) And $FFFFFF) + " " + Hex(ReadPixel(41,33) And $FFFFFF) + " " + Hex(ReadPixel(42,34) And $FFFFFF) + " " + Hex(ReadPixel(9,9) And $FFFFFF)
SetBuffer BackBuffer()
CloseMovie m2
Print "fertig"
End
