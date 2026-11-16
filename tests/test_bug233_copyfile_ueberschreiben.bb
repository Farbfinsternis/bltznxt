; BUG-233 - CopyFile ersetzt eine vorhandene Zieldatei.
;
; Gemeldet 2026-11-16 von einem Nutzer mit eigenem Projekt. Wie im Original
; (Win32 CopyFile mit bFailIfExists = FALSE) muss das Ziel ueberschrieben
; werden. Vorher brach std::filesystem::copy_file aus MinGWs libstdc++ mit
; "File exists" ab und das Ziel blieb alt. Ein kuerzerer Inhalt beim zweiten
; Kopieren zeigt, dass das Ziel ganz ersetzt und nicht nur ueberschrieben wird.
; .expected_stderr ist leer: keine Laufzeitmeldung.
q$ = "bin/test_bug233_quelle.txt"
z$ = "bin/test_bug233_ziel.txt"

f = WriteFile(q$) : WriteLine f, "NEU" : CloseFile f
f = WriteFile(z$) : WriteLine f, "ALT ALT ALT" : WriteLine f, "zweite Zeile" : CloseFile f

CopyFile q$, z$
f = ReadFile(z$)
Print ReadLine(f)
Print Eof(f)
CloseFile f
Print FileSize(z$) = FileSize(q$)

; noch einmal ueber das jetzt gleich grosse Ziel
f = WriteFile(q$) : WriteLine f, "NEUER" : CloseFile f
CopyFile q$, z$
f = ReadFile(z$)
Print ReadLine(f)
CloseFile f

DeleteFile q$
DeleteFile z$
Print FileType(z$)
