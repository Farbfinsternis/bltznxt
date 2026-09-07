; BUG-39 - SelectNode::semant lehnt ein Select ueber ein Objekt ab, bevor es
; irgendetwas anderes ansieht:
;   Type *ty=expr->sem_type;
;   if( ty->structType() ) ex( "Select cannot be used with objects" );
; Zahlen und Strings bleiben erlaubt, nur ein Type nicht.

Type Punkt
  Field x%
End Type

Local p.Punkt = New Punkt

; ein Objekt in einer Variablen
Select p
  Case Null
    Print "null"
  Default
    Print "objekt"
End Select

; ein Ausdruck, der ein Objekt liefert
Select First Punkt
  Default
    Print "erstes"
End Select
