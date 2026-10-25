; BUG-201 - ein Pfad mit Umlaut liess den Compiler abstuerzen.
;
; Pfade kommen in der ANSI-Codepage (argv, und so steht ein Include im
; Quelltext aus der Blitz-IDE - diese Datei ist Windows-1252). Unter MinGW
; las std::filesystem::path sie als UTF-8 und warf: "filesystem_error,
; Illegal byte sequence", Exitcode 3. Das Original uebersetzt das
; (gemessen, build/bug201_20261025). Hier liegt die eingebundene Datei in
; einem Ordner mit Umlaut.
Include "bug201_übung\grüße.bb"
Print Gruss$()
Print Len("äöü")
