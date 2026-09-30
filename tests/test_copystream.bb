; CopyStream (bb_socket.h): liest aus dem ersten Stream, bis er leer ist, und
; schreibt in den zweiten; der dritte Parameter ist die Puffergroesse.
;
; Am Original gemessen (2026-09-30, build/udp20260930/): Datei -> UDP-Stream, der eine
; ungelesene Nachricht hat, kopiert alle 10 Bytes (Fall 1). Abweichung vom Original: in eine
; Datei kopiert es dort nie etwas (gemessen mit WriteFile, OpenFile und einem UDP-Stream als
; Quelle), hier schon (Faelle 2 bis 4).
loop = 2130706433
a = CreateUDPStream(28721)
b = CreateUDPStream()
UDPTimeouts 300

f = WriteFile("__cs1.bin") : For i = 1 To 10 : WriteByte f, i : Next : CloseFile f

; 1) Datei -> UDP-Stream (mit ungelesener Nachricht), dann senden
WriteInt b, 5 : SendUDPMsg b, loop, 28721
r = RecvUDPMsg(a)
src = ReadFile("__cs1.bin")
CopyStream src, a
CloseFile src
SendUDPMsg a, loop, UDPStreamPort(b)
r = RecvUDPMsg(b)
Print "1 datei nach udp: " + ReadAvail(b) + " erstes " + ReadByte(b)

; 2) Datei -> Datei
src = ReadFile("__cs1.bin") : dst = WriteFile("__cs2.bin")
CopyStream src, dst
CloseFile src : CloseFile dst
Print "2 datei nach datei: " + FileSize("__cs2.bin")

; 3) mit kleinem Puffer, in Stuecken zu 3 Bytes (3 + 3 + 3 + 1)
src = ReadFile("__cs1.bin") : dst = WriteFile("__cs3.bin")
CopyStream src, dst, 3
CloseFile src : CloseFile dst
f = ReadFile("__cs3.bin") : s$ = ""
While Not Eof(f) : s = s + ReadByte(f) + " " : Wend
CloseFile f
Print "3 puffer 3: " + FileSize("__cs3.bin") + " | " + s

; 4) UDP-Nachricht -> Datei: was noch nicht gelesen ist
WriteInt b, 1 : WriteInt b, 2 : WriteInt b, 3 : SendUDPMsg b, loop, 28721
r = RecvUDPMsg(a)
ReadInt a
dst = WriteFile("__cs4.bin")
CopyStream a, dst
CloseFile dst
Print "4 udp nach datei: " + FileSize("__cs4.bin") + " rest " + ReadAvail(a) + " eof " + Eof(a)

DeleteFile "__cs1.bin" : DeleteFile "__cs2.bin" : DeleteFile "__cs3.bin" : DeleteFile "__cs4.bin"
