; Milestone 24 — File Open/Close/Seek

Local path$ = "bin/test_m24_tmp.dat"

; ----- WriteFile creates / truncates -----
Local fw = WriteFile(path)
If fw > 0 Then
    Print "WriteFile OK"
Else
    Print "WriteFile FAIL"
EndIf

; ----- FilePos at start = 0 -----
If FilePos(fw) = 0 Then
    Print "FilePos OK"
Else
    Print "FilePos FAIL"
EndIf

; ----- Eof on empty file = True -----
If Eof(fw) Then
    Print "Eof empty OK"
Else
    Print "Eof empty FAIL"
EndIf

; ----- ReadAvail on empty file = 0 -----
If ReadAvail(fw) = 0 Then
    Print "ReadAvail empty OK"
Else
    Print "ReadAvail empty FAIL"
EndIf

CloseFile fw
Print "CloseFile OK"

; ----- ReadFile opens existing file -----
Local fr = ReadFile(path)
If fr > 0 Then
    Print "ReadFile OK"
Else
    Print "ReadFile FAIL"
EndIf

; ----- SeekFile: absolute seek to pos 0 -----
SeekFile fr, 0
If FilePos(fr) = 0 Then
    Print "SeekFile OK"
Else
    Print "SeekFile FAIL"
EndIf

CloseFile fr

Print "DONE"
