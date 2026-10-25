; BUG-204 - GetKey() und GetMouse() warten nicht.
;
; Beide lesen im Original nur die Warteschlange und liefern 0, wenn nichts
; ansteht (gxDevice::getKey in bbinput.cpp); nur WaitKey/WaitMouse warten.
; Gemessen an Blitz3D 11.8 (build/bug204_20261025/leer): 100 Durchlaeufe ohne
; Eingabe, jeder Aufruf 0, zusammen weit unter einer Sekunde - genau diese
; Ausgabe. Bei uns hing schon der erste GetKey(), bis eine Taste kam; jede
; Spielschleife mit "key=GetKey()" stand in jedem Bild still (Skully/flares
; blieb schwarz).
Graphics 200,150,0,2
t=MilliSecs()
For i=1 To 100
	k=GetKey()
	m=GetMouse()
	If k<>0 Or m<>0 Then Exit
Next
Print k
Print m
Print i
Print (MilliSecs()-t) < 1000
