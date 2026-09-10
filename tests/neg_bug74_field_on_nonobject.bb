; BUG-74 - ein Feldzugriff braucht einen Objekttyp.
;
; Referenz: FieldVarNode::semant in compiler/varnode.cpp meldet
; "Variable must be a Type", sobald der Traeger keinen Strukturtyp hat.
;
; Am Original gemessen (2026-09-10): gemeldet wird nicht an der Variablen,
; sondern am Anfang der ANWEISUNG - "a = 1 : Print a\x" nennt Spalte 9, nicht
; 15. Der Zugriff steht hier deshalb eingerueckt, damit die Spalte etwas
; aussagt; mit dem Original abgeglichen ist "11:5".
a = 1
    Print a\x
