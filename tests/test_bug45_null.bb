; BUG-45 - die Gegenprobe: wo Null gueltig ist.
;
; Null hat in der Referenz einen eigenen Typ, StructType("Null") in
; compiler/type.cpp: er passt auf jedes Objekt und jedes Objekt auf ihn,
; aber auf nichts sonst. Bis 2026-09-15 kam Null hier als Ganzzahl 0 an; die
; Ablehnungsfaelle, die das verdeckt hat, liegen als neg_bug45_* daneben.

Type Punkt
  Field x
End Type

Type Kiste
  Field inhalt.Punkt
End Type

Function Finde.Punkt(wert)
  For p.Punkt = Each Punkt
    If p\x = wert Then Return p
  Next
  Return Null
End Function

Function Beschreibe$(p.Punkt)
  If p = Null Then Return "nichts"
  Return "punkt " + p\x
End Function

; --- 1) Zuweisen: Variable, Local, Feld ---
a.Punkt = New Punkt
a\x = 1
b.Punkt = New Punkt
b\x = 2
Local c.Punkt = Null
k.Kiste = New Kiste
Print "feld leer: " + (k\inhalt = Null)
k\inhalt = a
Print "feld belegt: " + (k\inhalt <> Null)
k\inhalt = Null
Print "feld wieder leer: " + (k\inhalt = Null)

; --- 2) Vergleichen, auf beiden Seiten ---
Print "c = Null: " + (c = Null)
Print "Null = c: " + (Null = c)
Print "a <> Null: " + (a <> Null)
Print "Null = Null: " + (Null = Null)

; --- 3) Als Argument und als Rueckgabewert ---
Print Beschreibe(Null)
Print Beschreibe(Finde(2))
Print Beschreibe(Finde(7))
If Finde(7) = Null Then Print "nicht gefunden"

; --- 4) Ende einer Liste ---
n = 0
p.Punkt = First Punkt
While p <> Null
  n = n + 1
  p = After p
Wend
Print "punkte: " + n

; --- 5) Delete Null ist gueltig und tut nichts ---
; Bis BUG-45 wurde daraus "0 = nullptr;", und g++ brach ab.
Delete Null
Print "delete null ok"
