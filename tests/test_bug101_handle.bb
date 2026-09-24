; BUG-101 - Handle und Object.
;
; Handle gibt einem Objekt beim ersten Aufruf die naechste Nummer (ab 1) und
; behaelt sie; Null und geloeschte Objekte haben 0. Object.T liefert das
; Objekt nur zu einer bekannten Nummer des richtigen Typs, sonst Null; Delete
; streicht die Nummer. Werte am Original gemessen (2026-09-24,
; build/types20260924).

Type T
Field x
End Type
Type U
Field y
End Type
a.T=New T : a\x=1 : b.T=New T : b\x=2 : u.U=New U
Print Handle(b) : Print Handle(a) : Print Handle b : Print Handle(u)
h=Handle(a)
Print (Object.U(h)=Null)
c.T=Object T h
Print c\x
Delete a
Print Handle(a) : Print (Object.T(h)=Null)
d.T=New T
Print Handle(d)
Print (Object.T(999)=Null)
Print Handle(Null)
End
