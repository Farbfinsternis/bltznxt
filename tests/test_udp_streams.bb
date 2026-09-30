; UDP-Streams (bb_socket.h): CreateUDPStream, SendUDPMsg, RecvUDPMsg und Verwandte.
;
; Am Original gemessen (2026-09-30, build/udp20260930/). Alles ueber 127.0.0.1
; im selben Programm, kein Netz noetig. Ein UDP-Stream ist ein Stream wie eine
; Datei: Write... sammelt die Nachricht, SendUDPMsg verschickt sie, RecvUDPMsg
; holt die naechste ein, Read..., ReadAvail und Eof arbeiten darauf.
Function P(t$)
	Print t
End Function

Function Nachricht(u, n)
	; eine Nachricht aus n Bytes (Zaehler) schreiben
	bk = CreateBank(n)
	For i = 0 To n - 1
		PokeByte bk, i, i Mod 251
	Next
	WriteBytes bk, u, 0, n
	FreeBank bk
End Function

loop = 2130706433
a = CreateUDPStream(28701)
P "1 erstellt: " + (a <> 0) + " port " + UDPStreamPort(a) + " ip " + UDPStreamIP(a)
b = CreateUDPStream()
P "2 freier port: " + (b <> 0) + " >0 " + (UDPStreamPort(b) > 0) + " anders " + (UDPStreamPort(b) <> 28701) + " ip " + UDPStreamIP(b)
P "3 port belegt: " + CreateUDPStream(28701)
P "4 leer: " + RecvUDPMsg(a) + " msgip " + UDPMsgIP(a) + " msgport " + UDPMsgPort(a) + " avail " + ReadAvail(a) + " eof " + Eof(a)

UDPTimeouts 200
t = MilliSecs() : r = RecvUDPMsg(a) : dt = MilliSecs() - t
P "5 warten: " + r + " dauer " + (dt >= 150 And dt <= 450)
UDPTimeouts 0
t = MilliSecs() : r = RecvUDPMsg(a) : dt = MilliSecs() - t
P "5 nicht warten: " + r + " dauer " + (dt < 50)

WriteInt b, 123456
WriteString b, "Hallo"
WriteByte b, 7
WriteFloat b, 1.5
WriteLine b, "zeile"
SendUDPMsg b, loop, 28701
UDPTimeouts 500
ip = RecvUDPMsg(a)
P "6 empfangen: " + DottedIP(ip) + " msgip " + DottedIP(UDPMsgIP(a)) + " msgport " + (UDPMsgPort(a) = UDPStreamPort(b))
P "6 avail: " + ReadAvail(a) + " eof " + Eof(a)
P "6 lesen: " + ReadInt(a) + " " + ReadString(a) + " " + ReadByte(a) + " " + ReadFloat(a) + " " + ReadLine(a)
P "6 danach: avail " + ReadAvail(a) + " eof " + Eof(a)
P "6 ueber das Ende: " + ReadInt(a) + " " + ReadByte(a) + " eof " + Eof(a)

; ohne Zielport geht die Nachricht an den eigenen Port des Streams
WriteInt b, 42
SendUDPMsg b, loop
ip = RecvUDPMsg(b)
P "7 ohne port: " + DottedIP(ip) + " " + ReadInt(b) + " msgport " + (UDPMsgPort(b) = UDPStreamPort(b))
P "7 a leer: " + RecvUDPMsg(a)

; Nachrichten bleiben getrennt und in der Reihenfolge
WriteInt b, 1 : SendUDPMsg b, loop, 28701
WriteInt b, 2 : WriteInt b, 3 : SendUDPMsg b, loop, 28701
ip = RecvUDPMsg(a) : x = ReadInt(a) : e1 = Eof(a)
ip2 = RecvUDPMsg(a) : y = ReadInt(a) : z = ReadInt(a) : e2 = Eof(a)
P "8 reihenfolge: " + x + " eof " + e1 + " | " + y + " " + z + " eof " + e2 + " | " + RecvUDPMsg(a)

; der Schreibpuffer ist nach dem Senden leer; eine leere Nachricht kommt an
WriteInt b, 9 : SendUDPMsg b, loop, 28701
SendUDPMsg b, loop, 28701
ip = RecvUDPMsg(a) : P "9 erste: avail " + ReadAvail(a)
ip = RecvUDPMsg(a) : P "9 zweite: ip " + (ip <> 0) + " avail " + ReadAvail(a) + " eof " + Eof(a)

; nach einem Lesefehlschlag: was bleibt vom Lesepuffer?
WriteInt b, 77 : SendUDPMsg b, loop, 28701
ip = RecvUDPMsg(a)
P "10 vor leer: " + ReadAvail(a)
ip = RecvUDPMsg(a)
P "10 nach leer: ip " + ip + " avail " + ReadAvail(a) + " msgip " + (UDPMsgIP(a) <> 0) + " msgport " + (UDPMsgPort(a) <> 0) + " lesen " + ReadInt(a)

; zwei Schreibstroeme: was der Stream schreibt, ist nicht das, was er las
WriteInt b, 5 : SendUDPMsg b, loop, 28701
ip = RecvUDPMsg(a)
WriteInt a, 6
SendUDPMsg a, loop, UDPStreamPort(b)
P "11 lesen nach schreiben: " + ReadInt(a) + " eof " + Eof(a)
ip = RecvUDPMsg(b)
P "11 antwort: " + DottedIP(ip) + " " + ReadInt(b) + " msgport " + UDPMsgPort(b) + " a " + UDPStreamPort(a)

; Groessen
Dim gr(9)
gr(0) = 1 : gr(1) = 100 : gr(2) = 1000 : gr(3) = 1400 : gr(4) = 2048 : gr(5) = 4000 : gr(6) = 4096 : gr(7) = 4100 : gr(8) = 8000 : gr(9) = 60000
s$ = ""
For i = 0 To 9
	Nachricht b, gr(i)
	SendUDPMsg b, loop, 28701
	ip = RecvUDPMsg(a)
	s = s + gr(i) + "->" + ReadAvail(a) + " "
Next
P "12 groessen: " + s

; Inhalt einer langen Nachricht
Nachricht b, 3000
SendUDPMsg b, loop, 28701
ip = RecvUDPMsg(a)
bk = CreateBank(3000)
n = ReadBytes(bk, a, 0, 3000)
ok = True
For i = 0 To n - 1
	If PeekByte(bk, i) <> i Mod 251 Then ok = False
Next
P "13 inhalt: " + n + " " + ok + " eof " + Eof(a)

; eine Nachricht von a nach b weiterreichen
WriteInt b, 31 : WriteString b, "kopie" : SendUDPMsg b, loop, 28701
ip = RecvUDPMsg(a)
SendUDPMsg a, loop, UDPStreamPort(b)
ip = RecvUDPMsg(b)
P "14 zurueck: " + ReadInt(b) + " " + ReadString(b)

CloseUDPStream a
CloseUDPStream b
c = CreateUDPStream(28701)
P "15 nach close: " + (c <> 0) + " " + UDPStreamPort(c)
CloseUDPStream c
End
