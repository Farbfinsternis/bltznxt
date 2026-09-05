; BUG-17 - Blitz3D erlaubt Global nur auf der obersten Ebene des Hauptprogramms:
; parseStmtSeq bricht mit "'Global' can only appear in main program" ab, sobald
; scope != STMTS_PROG. Vorher erzeugte das hier stillschweigend C++, das nicht
; uebersetzt ("'var_g' was not declared in this scope").
Local a% = 1
If a = 1 Then
  Global g% = 7
EndIf
