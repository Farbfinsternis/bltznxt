; BUG-102 - Ein Feld oder ein Array-Element als Zaehler von For Each.
;
; Das Original liest den Zaehler mit parseVar() wie jedes Ziel einer
; Zuweisung; wir lehnten beide Formen ab. Werte am Original gemessen
; (2026-09-24, build/types20260924).

Type T
Field x, c.T
End Type
For i=1 To 3 : t.T=New T : t\x=i : Next

p.T=First T
s$="" : For p\c=Each T : s=s+p\c\x : Next
Print "feld: "+s+" danach Null: "+(p\c=Null)

Dim a.T(1)
s$="" : For a(1)=Each T : s=s+a(1)\x : Next
Print "dim: "+s
End
