; TCP-Streams und Namensaufloesung (bb_socket.h).
;
; Am Original gemessen (2026-09-17, build/tcp20260917/). Verbindung zu einem
; eigenen Server im selben Programm ueber 127.0.0.1 - kein Netz noetig. Die
; Lese- und Schreibbefehle laufen ueber dieselben Wege wie bei Dateien.
Print "1 dottedip: " + DottedIP(0) + " " + DottedIP(2130706433) + " " + DottedIP(-1) + " " + DottedIP(16909060)
n = CountHostIPs("localhost")
Print "2 localhost: " + (n > 0) + " " + DottedIP(HostIP(1))
Print "3 unbekannt: " + CountHostIPs("gibtsnicht.invalid")

TCPTimeouts 2000,200
server = CreateTCPServer(28642)
Print "4 server: " + (server <> 0)
Print "5 leer annehmen: " + AcceptTCPStream(server)

client = OpenTCPStream("127.0.0.1",28642)
Print "6 verbunden: " + (client <> 0) + " port " + (TCPStreamPort(client) > 0) + " ip " + DottedIP(TCPStreamIP(client))
dienst = AcceptTCPStream(server)
Print "7 angenommen: " + (dienst <> 0) + " ip " + DottedIP(TCPStreamIP(dienst))

WriteLine client,"Hallo Welt"
WriteLine client,"zweite Zeile"
Print "8 empfangen: " + ReadLine(dienst) + " | " + ReadLine(dienst)

WriteByte client,65
WriteInt client,123456
WriteFloat client,0.5
WriteString client,"xy"
Delay 100
Print "9 bytes da: " + (ReadAvail(dienst) > 0)
Print "10 lesen: " + ReadByte(dienst) + " " + ReadInt(dienst) + " " + ReadFloat(dienst) + " " + ReadString(dienst)

Print "11 eof offen: " + Eof(dienst)
CloseTCPStream client
Delay 100
Print "12 eof nach schliessen: " + Eof(dienst)
CloseTCPStream dienst
CloseTCPServer server

Print "13 kein server: " + OpenTCPStream("127.0.0.1",28643)
Print "14 unbekannter host: " + OpenTCPStream("gibtsnicht.invalid",80)
End
