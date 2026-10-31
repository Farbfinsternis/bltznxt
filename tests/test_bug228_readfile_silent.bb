; BUG-228: ReadFile/OpenFile auf eine fehlende Datei liefern 0 - ohne Meldung,
; wie im Original. Programme pruefen so, ob eine Datei da ist.
f = ReadFile("gibt_es_nicht_bug228.txt")
Print "ReadFile: " + f
f = OpenFile("gibt_es_nicht_bug228.txt")
Print "OpenFile: " + f
If Not ReadFile("gibt_es_nicht_bug228.txt") Then Print "fehlt"
