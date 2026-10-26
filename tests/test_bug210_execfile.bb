; BUG-210 - ExecFile kehrt sofort zurueck und startet ueber die Shell.
;
; Das Original trennt Befehl und Parameter und ruft ShellExecute; gemessen an
; Blitz3D 11.8 (build/bug210_20261025/exec): genau diese Ausgabe. Vorher hielt
; std::system das Programm an, bis der Befehl fertig war (hier etwa 2 s).
DeleteFile "exec_out.txt"
t = MilliSecs()
ExecFile "cmd /c ping -n 3 127.0.0.1 > exec_out.txt"
Print (MilliSecs() - t) < 1000
For i = 1 To 100
	If FileType("exec_out.txt") = 1 And FileSize("exec_out.txt") > 100 Then Exit
	Delay 100
Next
Print FileType("exec_out.txt")
Delay 300
DeleteFile "exec_out.txt"
