; BUG-31 - Blitz3D liest hinter jedem Namen einen Type-Tag, auch hinter einem
; Feldnamen (parseVar(), der "\"-Zweig). FieldVarNode::semant sieht ihn dann
; gar nicht an und nimmt den Typ aus der Felddeklaration - der Tag wird also
; gelesen und verworfen, ein widersprechender eingeschlossen.
; Vorher lehnte der Parser jede getaggte Schreibstelle ab.

Type P
  Field n%
  Field f#
  Field s$
End Type
Local p.P = New P

p\n% = 7
p\f# = 1.5
p\s$ = "text"
Print "getaggt geschrieben: " + p\n + " " + p\f + " " + p\s

For p\n% = 1 To 3
Next
Print "For mit getaggtem Feld: " + p\n

For p\f# = 1.0 To 2.0 Step 0.5
Next
Print "For mit getaggtem Float-Feld: " + p\f

; Der Tag wird gelesen und verworfen — auch ein widersprechender, genau wie in
; FieldVarNode::semant, das den Tag gar nicht ansieht.
p\f$ = 2.5
Print "widersprechender Tag: " + p\f
Print "widersprechend gelesen: " + p\f%
