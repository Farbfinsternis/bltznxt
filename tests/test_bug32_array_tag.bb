; BUG-32 - bei Arrays prueft Blitz3D den Type-Tag gegen den Elementtyp, anders
; als bei Feldern (BUG-31). ArrayVarNode::semant loest den Tag mit findType()
; auf ("%" -> int, "#" -> float, "$" -> string) und meldet, wenn er nicht zum
; Elementtyp passt. Ein passender Tag muss weiterhin durchgehen - das ist die
; eigentliche Gefahr an dieser Aenderung.

Dim zahlen(3)
Dim worte$(2)
Dim kommas#(2)

zahlen%(0) = 7
Print "passender Tag: " + zahlen%(0)

worte$(0) = "hallo"
Print "string-Array: " + worte$(0)

kommas#(0) = 1.5
Print "float-Array: " + kommas#(0)

For zahlen%(1) = 1 To 3
Next
Print "For mit Tag: " + zahlen(1)

Print "ohne Tag unveraendert: " + zahlen(0) + " " + worte(0) + " " + kommas(0)
