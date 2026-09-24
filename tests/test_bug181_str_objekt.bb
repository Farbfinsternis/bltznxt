; BUG-181 - Str auf einem Objekt.
;
; Das Original schreibt die Felder in eckigen Klammern (_bbObjToStr):
; Zeichenketten in Anfuehrungszeichen, Objektfelder verschachtelt, Null und
; geloeschte Objekte als [NULL], das Ausgangsobjekt als [ROOT], Array-Felder
; als ???. Bei uns uebersetzte Str(obj) nicht. Werte am Original gemessen
; (2026-09-24, build/types20260924).

Type T
Field x, f#, s$, o.T, a[2]
End Type
p.T=New T : p\x=1 : p\f=2.5 : p\s="hi"
Print Str(p)
p\o=p : Print Str(p)
q.T=New T : q\x=7 : p\o=q : Print Str(p)
q\s="" : Print Str(q)
Delete q : Print Str(p)
n.T=Null : Print Str(n)
End
