; Test Filesystem
Print "Testing Filesystem..."

testDir$ = "test_fs_dir"
If FileType(testDir$) = 2 Then DeleteDir testDir$

CreateDir testDir$
If FileType(testDir$) <> 2 Then RuntimeError "Failed to create directory"

file$ = testDir$ + "/test_file.txt"
f = WriteFile(file$)
WriteString f, "Hello"
CloseFile f

If FileType(file$) <> 1 Then RuntimeError "Failed to create file"

dir = ReadDir(testDir$)
If dir = 0 Then RuntimeError "Failed to read directory"

found = 0
Repeat
	f$ = NextFile(dir)
	If f$ = "" Then Exit
	If f$ = "test_file.txt" Then found = 1
	Print "Found: " + f$
Forever

CloseDir dir

If found = 0 Then 
	Print "FAIL: File not found in directory listing"
Else
	Print "PASS: File found"
EndIf

DeleteFile file$
DeleteDir testDir$

If FileType(testDir$) <> 0 Then Print "FAIL: Directory not deleted" Else Print "PASS: Directory deleted"

End
