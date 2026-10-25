; BUG-80 - eine neue Variable ohne Tag ist immer ein Int, egal was rechts steht.
;
; Am laufenden Original gemessen (Blitz3D 11.8, build/bug80_20261025/pos):
; varnode.cpp legt das ungetaggte Ziel als int an, der Wert wandelt hinein.
; Vorher hat unser Analyzer den Typ der rechten Seite uebernommen - "s="12" :
; Print s-1" wurde mit "Operator cannot be applied to strings" abgelehnt. Das
; sperrte fuenf Installationsbeispiele (mode=Input$(...) : mode=mode-1 in den
; start.bb-Menues, an_step#=360.0/n_dudes in mak/collide).
x=1.5
Print x
Print x%
w=w+1.5
Print w
s="12"
Print s-1
Print s*2
a="abc"
Print a
b="7"
Print b+"a"
z=2.7
Print z/2
k$="5"
m=k$
Print m*2
f=Sqr(10)
Print f
Print f+0.5
Print fn()
r=ReadMe$()
Print r+1

Function fn()
	v="3"
	Return v+1
End Function

Function ReadMe$()
	Return "41"
End Function
