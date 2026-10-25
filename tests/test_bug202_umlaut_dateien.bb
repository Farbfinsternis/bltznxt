; BUG-202 - Datei- und Ordnernamen mit Umlaut.
;
; Ein Blitz-String ist eine Bytefolge in der ANSI-Codepage; diese Datei ist
; Windows-1252 wie aus der Blitz-IDE. std::filesystem las solche Namen unter
; MinGW als UTF-8 und warf: das Programm brach beim ersten FileType,
; ReadDir, CopyFile oder Loader mit filesystem_error ab. Das Original liefert
; genau diese Ausgabe (gemessen, build/bug202_20261025). Dazu CurrentDir mit
; abschliessendem Backslash wie im Original. Alles in einem eigenen Ordner,
; der am Ende wieder verschwindet.
Graphics 64,64,0,2
CreateDir "bug202_Übung"
ChangeDir "bug202_Übung"
Print Right(CurrentDir(), 14)
f = WriteFile("äpfel.txt")
WriteLine f, "inhalt äöü"
CloseFile f
Print FileType("äpfel.txt")
Print FileSize("äpfel.txt")
r = ReadFile("äpfel.txt")
Print Len(ReadLine(r))
CloseFile r
CopyFile "äpfel.txt", "Büd.txt"
Print FileType("Büd.txt")
img = CreateImage(4,4)
SaveImage img, "Größe.bmp"
Print FileType("Größe.bmp")
Print LoadImage("Größe.bmp") <> 0
CreateDir "Ördner"
Print FileType("Ördner")
z = 0
d = ReadDir(".")
Repeat
	n$ = NextFile$(d)
	If n$ = "" Then Exit
	If n$ = "äpfel.txt" Or n$ = "Büd.txt" Or n$ = "Größe.bmp" Or n$ = "Ördner" Then z = z + 1
Forever
CloseDir d
Print z
DeleteDir "Ördner"
Print FileType("Ördner")
DeleteFile "äpfel.txt"
DeleteFile "Büd.txt"
DeleteFile "Größe.bmp"
Print FileType("äpfel.txt")
ChangeDir ".."
DeleteDir "bug202_Übung"
Print FileType("bug202_Übung")
