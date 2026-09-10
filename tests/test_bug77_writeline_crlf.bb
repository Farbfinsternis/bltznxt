; BUG-77 - WriteLine beendet eine Zeile mit CRLF, nicht mit LF.
;
; Referenz: bbWriteLine in bbruntime/bbstream.cpp schreibt den String und
; danach ausdruecklich s->write( "\r\n",2 ) - zwei Bytes, unabhaengig vom
; Betriebssystem, denn ein Blitz3D-Stream ist immer binaer.
;
; Der Test liest die geschriebenen Bytes wieder ein, statt nur die Ausgabe
; zu vergleichen: die Testhuelle vergleicht ueber $(...) und wuerde einen
; Unterschied im Zeilenende gar nicht sehen. Alle Werte sind am laufenden
; Original gemessen (2026-09-10).

f = WriteFile("__test_bug77__.tmp")
WriteLine f, "AB"
WriteLine f, ""
WriteLine f, "C"
CloseFile f

; Erwartet: 65 66 13 10 13 10 67 13 10 - auch die leere Zeile bekommt CRLF.
f = ReadFile("__test_bug77__.tmp")
b$ = ""
While Not Eof(f)
	If b$ <> "" Then b$ = b$ + " "
	b$ = b$ + Str(ReadByte(f))
Wend
CloseFile f
Print "bytes: " + b$

; ReadLine muss das unveraendert zurueckgeben - es verwirft jedes CR und
; bricht bei LF ab, genau wie bbReadLine. Der Rundgang darf nichts kosten.
f = ReadFile("__test_bug77__.tmp")
Print "z1=[" + ReadLine(f) + "]"
Print "z2=[" + ReadLine(f) + "]"
Print "z3=[" + ReadLine(f) + "]"
CloseFile f

; Das Zeilenende zaehlt in der Dateiposition mit: 2 Zeichen + 2 Bytes.
f = WriteFile("__test_bug77b__.tmp")
WriteLine f, "AB"
p = FilePos(f)
CloseFile f
Print "filepos nach WriteLine AB = " + Str(p)

DeleteFile "__test_bug77__.tmp"
DeleteFile "__test_bug77b__.tmp"
