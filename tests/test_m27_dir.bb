; Milestone 27 — Directory & File Info

; ----- FileType -----
If FileType(".") = 2 Then
    Print "FileType dir OK"
Else
    Print "FileType dir FAIL"
EndIf

If FileType("bin/blitzcc.exe") = 1 Then
    Print "FileType file OK"
Else
    Print "FileType file FAIL"
EndIf

If FileType("nonexistent_xyz_99") = 0 Then
    Print "FileType none OK"
Else
    Print "FileType none FAIL"
EndIf

; ----- FileSize -----
Local sz% = FileSize("bin/blitzcc.exe")
If sz > 0 Then
    Print "FileSize OK"
Else
    Print "FileSize FAIL"
EndIf

; ----- CurrentDir -----
Local cd$ = CurrentDir()
If Len(cd) > 0 Then
    Print "CurrentDir OK"
Else
    Print "CurrentDir FAIL"
EndIf

; ----- CreateDir / DeleteDir -----
CreateDir "bin/test_m27_tmpdir"
If FileType("bin/test_m27_tmpdir") = 2 Then
    Print "CreateDir OK"
Else
    Print "CreateDir FAIL"
EndIf

DeleteDir "bin/test_m27_tmpdir"
If FileType("bin/test_m27_tmpdir") = 0 Then
    Print "DeleteDir OK"
Else
    Print "DeleteDir FAIL"
EndIf

; ----- CopyFile / DeleteFile -----
; Create source file
Local fw = WriteFile("bin/test_m27_src.dat")
CloseFile fw

CopyFile "bin/test_m27_src.dat", "bin/test_m27_dst.dat"
If FileType("bin/test_m27_dst.dat") = 1 Then
    Print "CopyFile OK"
Else
    Print "CopyFile FAIL"
EndIf

DeleteFile "bin/test_m27_src.dat"
DeleteFile "bin/test_m27_dst.dat"
If FileType("bin/test_m27_dst.dat") = 0 Then
    Print "DeleteFile OK"
Else
    Print "DeleteFile FAIL"
EndIf

; ----- ReadDir / NextFile / CloseDir -----
Local dir = ReadDir("bin")
Local entry$ = NextFile(dir)
If Len(entry) > 0 Then
    Print "ReadDir/NextFile OK"
Else
    Print "ReadDir/NextFile FAIL"
EndIf
CloseDir dir

Print "DONE"
