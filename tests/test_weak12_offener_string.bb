; WEAK-12 - ein String ohne Schlusszeichen endet am Zeilenende.
;
; Das Original (toker.cpp) nimmt den String bis zum Zeilenende, und
; parser.cpp schneidet wie bei jedem String das erste und das letzte Zeichen
; des Tokens ab - also hier das letzte Zeichen des Inhalts. Gemessen an
; Blitz3D 11.8 mit LF und CRLF (build/weak12_20261025): "abc wird "ab",
; "def wird "de". Vorher lehnte unser Lexer das ab ("unclosed string
; literal"), und samples/halo/Lightmap uebersetzte nicht.
s$ = "abc
Print Len(s)
Print "<"+s+">"
t$ = "x" + "def
Print t
Print Len("")
