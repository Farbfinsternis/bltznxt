; BUG-215 - ReadDir liefert "." und ".." wie das Original.
;
; Blitz3D liest Ordner ueber FindFirstFile/FindNextFile: in jedem
; gewoehnlichen Ordner kommen "." und ".." zuerst, auch wenn er leer ist, im
; Stamm eines Laufwerks nicht. Gemessen an Blitz3D 11.8
; (build/bug215_20261025/dots). Bei uns fehlten beide immer, und Code, der die
; ersten zwei Eintraege ueberspringt, verlor zwei echte Dateien.
CreateDir "bug215_leer"
d = ReadDir("bug215_leer")
Print NextFile$(d)
Print NextFile$(d)
Print "<" + NextFile$(d) + ">"
CloseDir d
d = ReadDir("bug215_leer\")
Print NextFile$(d)
CloseDir d
DeleteDir "bug215_leer"
d = ReadDir(Left(CurrentDir(), 3))
Print NextFile$(d) <> "."
CloseDir d
Print ReadDir("bug215_gibtsnicht")
