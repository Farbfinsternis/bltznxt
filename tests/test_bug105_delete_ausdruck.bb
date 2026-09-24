; BUG-105 - Delete und Insert mit jedem Objektausdruck.
;
; Der Emitter brauchte fuer Delete den Typnamen und kannte ihn fuer ein Feld,
; ein Element eines lokalen festen Arrays oder "New T" nicht: dann wurde nur
; das Feld genullt (das Objekt blieb in der Liste) oder das C++ scheiterte.
; Insert tat in diesem Fall still gar nichts. Werte am Original gemessen
; (2026-09-24, build/types20260924).

Type T
Field x, c.T
End Type

Function Zahl()
	n=0 : For t.T=Each T : n=n+1 : Next : Return n
End Function

p.T=New T : p\c=New T : p\c\x=5
Delete p\c
Print "feld: "+Zahl()+" "+(p\c=Null)

Delete New T
Print "new: "+Zahl()

Local b.T[2]
b[0]=New T
Delete b[0]
Print "fest: "+Zahl()

Dim a.T(2)
a(1)=New T
Delete a(1)
Print "dim: "+Zahl()

Delete Each T
For i=1 To 3 : o.T=New T : o\x=i : Next
q.T=New T : q\c=Last T
Insert q\c Before First T
s$="" : For t.T=Each T : s=s+t\x : Next
Print "insert: "+s
End
