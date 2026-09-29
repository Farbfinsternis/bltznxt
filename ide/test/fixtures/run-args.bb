; Schreibt seine Kommandozeile neben sich: zeigt Argumente und Arbeitsordner (= Ordner der Quelle)
f = WriteFile("run-args.txt")
WriteLine f, CommandLine$()
CloseFile f
End
