; Das Blitz3D-Repo gibt die Spracharchitektur vor: Parser::parseTypeTag() kennt
; nur "%", "#", "$" und ".Name"; "!" kommt in toker.cpp weder als Keyword noch
; als Sonderzeichen vor. Der frueher als Float-Tag akzeptierte Ausrufezeichen-
; Suffix ist damit verworfen.
Local f! = 3.14
