; Milestone 25 + 26 — File Read/Write Primitives

Local path$ = "bin/test_m25_m26_tmp.dat"

; ----- Write all primitive types -----
Local fw = WriteFile(path)
WriteByte   fw, 255
WriteShort  fw, 1000
WriteInt    fw, 1234567
WriteFloat  fw, 3.14
WriteString fw, "hello"
WriteLine   fw, "world"
CloseFile fw
Print "Write OK"

; ----- Read back and verify -----
Local fr = ReadFile(path)

Local b% = ReadByte(fr)
If b = 255 Then
    Print "ReadByte OK"
Else
    Print "ReadByte FAIL"
EndIf

Local s% = ReadShort(fr)
If s = 1000 Then
    Print "ReadShort OK"
Else
    Print "ReadShort FAIL"
EndIf

Local i% = ReadInt(fr)
If i = 1234567 Then
    Print "ReadInt OK"
Else
    Print "ReadInt FAIL"
EndIf

Local f# = ReadFloat(fr)
If f > 3.13 And f < 3.15 Then
    Print "ReadFloat OK"
Else
    Print "ReadFloat FAIL"
EndIf

Local str$ = ReadString(fr)
If str = "hello" Then
    Print "ReadString OK"
Else
    Print "ReadString FAIL"
EndIf

Local line$ = ReadLine(fr)
If line = "world" Then
    Print "ReadLine OK"
Else
    Print "ReadLine FAIL"
EndIf

; Eof should be true now
If Eof(fr) Then
    Print "Eof after read OK"
Else
    Print "Eof after read FAIL"
EndIf

CloseFile fr
Print "DONE"
