; Das Blitz3D-Repo gibt die Spracharchitektur vor: parseStmtSeq laesst Global
; nur bei scope == STMTS_PROG zu ("'Global' can only appear in main program").
; Ein Funktionsrumpf ist das nicht. Die frueher geplante Erweiterung, Global
; auch dort zuzulassen, ist damit verworfen.
Function Setz()
  Global ausFunktion% = 3
End Function
