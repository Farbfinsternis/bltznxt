; Milestone 29 — Bank Peek/Poke

Local b = CreateBank(16)

; ----- PokeByte / PeekByte -----
PokeByte b, 0, 255
If PeekByte(b, 0) = 255 Then
    Print "PokeByte/PeekByte OK"
Else
    Print "PokeByte/PeekByte FAIL"
EndIf

; ----- PokeShort / PeekShort -----
PokeShort b, 1, 1000
If PeekShort(b, 1) = 1000 Then
    Print "PokeShort/PeekShort OK"
Else
    Print "PokeShort/PeekShort FAIL"
EndIf

; ----- PokeInt / PeekInt -----
PokeInt b, 4, 12345
If PeekInt(b, 4) = 12345 Then
    Print "PokeInt/PeekInt OK"
Else
    Print "PokeInt/PeekInt FAIL"
EndIf

; ----- PokeFloat / PeekFloat -----
PokeFloat b, 8, 3.14
Local f# = PeekFloat(b, 8)
If f > 3.13 And f < 3.15 Then
    Print "PokeFloat/PeekFloat OK"
Else
    Print "PokeFloat/PeekFloat FAIL"
EndIf

FreeBank b
Print "DONE"
