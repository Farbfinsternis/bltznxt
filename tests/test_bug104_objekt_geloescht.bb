; BUG-104 - Geloeschte Type-Objekte wie im Original.
;
; Das Original zaehlt Referenzen; "Delete" leert nur die Felder, das Objekt
; bleibt als Huelle in der Liste, bis die letzte Referenz verschwindet. Jede
; Referenz darauf ist gleich Null, und alle Wege durch die Liste ueberspringen
; es. Bei uns wurde der Speicher sofort freigegeben und nur die eine Variable
; genullt: ein Alias blieb ungleich Null und zeigte auf freien Speicher.
; Werte am Original gemessen (2026-09-24, build/obj20260924/werte.bb).

Type T
Field x
Field s$
End Type

; Alias nach Delete
p.T=New T : p\x=1
q.T=p
Delete p
Print "A p=Null "+(p=Null)+" q=Null "+(q=Null)+" p=q "+(p=q)

; zwei geloeschte Objekte sind gleich
a.T=New T : a\x=2
b.T=New T : b\x=3
Delete a : Delete b
Print "B a=b "+(a=b)+" a<>b "+(a<>b)

; For Each, After und Before ueberspringen geloeschte Objekte
Delete Each T
t1.T=New T : t1\x=10
t2.T=New T : t2\x=20
t3.T=New T : t3\x=30
z.T=t2
Delete t2
n=0 : s$=""
For e.T=Each T : n=n+1 : s=s+e\x+"," : Next
Print "C each n="+n+" "+s
h1.T=After z : h2.T=Before z : h3.T=After t1 : h4.T=Before t3
Print "C after(z)="+h1\x+" before(z)="+h2\x+" after(t1)="+h3\x+" before(t3)="+h4\x

; First und Last ebenso
zf.T=t1 : Delete t1
zl.T=t3 : Delete t3
Print "D first=Null "+(First T=Null)+" last=Null "+(Last T=Null)

; ein zweites Delete tut nichts
Delete zf
Print "E ok"

; Nachfolger im Rumpf loeschen
Delete Each T
For i=1 To 4 : o.T=New T : o\x=i : Next
For e.T=Each T
	If e\x=2 Then k.T=After e : Delete k
Next
n=0 : s$=""
For e.T=Each T : n=n+1 : s=s+e\x+"," : Next
Print "F n="+n+" "+s

; nach dem letzten Durchlauf haelt die Variable Null
For e.T=Each T : Next
Print "G e=Null "+(e=Null)

; Laufobjekt und Nachfolger im Rumpf loeschen
Delete Each T
For i=1 To 5 : o.T=New T : o\x=i : Next
s=""
For e.T=Each T
	s=s+e\x+","
	If e\x=2 Then nx.T=After e : Delete nx : Delete e
Next
Print "H "+s

; ein eingefuegtes geloeschtes Objekt bleibt unsichtbar
Delete Each T
t1=New T : t1\x=1 : t2=New T : t2\x=2
zz.T=t2 : Delete t2
Insert zz Before t1
s="" : For e.T=Each T : s=s+e\x+"," : Next
Print "I "+s

; Delete eines Parameters
Delete Each T
w.T=New T : w\x=7
j=Loesche(w)
Print "J "+j+" w=Null "+(w=Null)
End

Function Loesche(p.T)
	Delete p
	Return p=Null
End Function
