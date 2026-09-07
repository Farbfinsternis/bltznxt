; BUG-44, zweite Haelfte - Formen, die das Original erlaubt und die wir bis
; 2026-09-07 abgelehnt haben. Geprueft wird, was ohne Fenster und ohne
; angeschlossenen Joystick beobachtbar ist.

; --- CreateBank ohne Groesse: "CreateBank ( [size] )" ---
Local b = CreateBank()
Print "leere bank: " + Str(BankSize(b))
ResizeBank b, 4
PokeInt b, 0, 123
Print "nach resize: " + Str(PeekInt(b, 0))
FreeBank b

; --- SeekFile liefert die erreichte Position: "SeekFile ( file_stream,pos )" ---
Local aus = WriteFile("bug44_opt.bin")
WriteInt aus, 7
WriteInt aus, 8
CloseFile aus
Local ein = ReadFile("bug44_opt.bin")
Print "seek auf 4: " + Str(SeekFile(ein, 4))
Print "dort steht: " + Str(ReadInt(ein))
CloseFile ein
DeleteFile "bug44_opt.bin"

; --- Joystick ohne Port: "JoyX# ( [port] )", "FlushJoy" ohne Parameter ---
Print "joytype: " + Str(JoyType())
Print "joyx: " + Str(JoyX())
Print "joyhat: " + Str(JoyHat())
FlushJoy
Print "flushjoy ohne port: ok"

; --- WaitKey liefert einen Tastencode: "WaitKey ( )" ---
; Ohne Terminal an stdin kehrt es sofort mit 0 zurueck.
Local taste = WaitKey()
Print "waitkey: " + Str(taste)

Print "DONE"
